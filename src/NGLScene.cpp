#include <ngl/Random.h>
#include "ctime"

#include "NGLScene.h"
#include <QGuiApplication>
#include <QMouseEvent>

#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/NGLStream.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOPrimitives.h>
#include <QTimerEvent>

#include <vector>

#include <omp.h>
#include <thread>

NGLScene::NGLScene()
{
  setTitle( "Qt5 Simple NGL Demo" );
  _numParticles=5;


}


NGLScene::~NGLScene()
{
  std::cout << "Shutting down NGL, removing VAO's and Shaders\n";
}



void NGLScene::resizeGL( int _w, int _h )
{
  m_cam.setShape( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}


void NGLScene::initializeGL()
{
  // we must call that first before any other GL commands to load and link the
  // gl commands from the lib, if that is not done program will crash
  ngl::NGLInit::instance();
  glClearColor( 0.4f, 0.4f, 0.4f, 1.0f ); // Grey Background
  // enable depth testing for drawing
  glEnable( GL_DEPTH_TEST );
// enable multisampling for smoother drawing
#ifndef USINGIOS_
  glEnable( GL_MULTISAMPLE );
#endif
  // now to load the shader and set the values
  // grab an instance of shader manager
//  ngl::ShaderLib* shader = ngl::ShaderLib::instance();
//  // we are creating a shader called Phong to save typos
//  // in the code create some constexpr
//  constexpr auto shaderProgram = "Phong";
//  constexpr auto vertexShader  = "PhongVertex";
//  constexpr auto fragShader    = "PhongFragment";
//  // create the shader program
//  shader->createShaderProgram( shaderProgram );
//  // now we are going to create empty shaders for Frag and Vert
//  shader->attachShader( vertexShader, ngl::ShaderType::VERTEX );
//  shader->attachShader( fragShader, ngl::ShaderType::FRAGMENT );
//  // attach the source
//  shader->loadShaderSource( vertexShader, "shaders/PhongVertex.glsl" );
//  shader->loadShaderSource( fragShader, "shaders/PhongFragment.glsl" );
//  // compile the shaders
//  shader->compileShader( vertexShader );
//  shader->compileShader( fragShader );
//  // add them to the program
//  shader->attachShaderToProgram( shaderProgram, vertexShader );
//  shader->attachShaderToProgram( shaderProgram, fragShader );


//  // now we have associated that data we can link the shader
//  shader->linkProgramObject( shaderProgram );
//  // and make it active ready to load values
//  ( *shader )[ shaderProgram ]->use();


  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  constexpr auto Point="Point";
  constexpr auto PointVertex="PointVertex";
  constexpr auto PointFragment="PointFragment";

  // we are creating a shader called Phong
  shader->createShaderProgram(Point);
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader(PointVertex,ngl::ShaderType::VERTEX);
  shader->attachShader(PointFragment,ngl::ShaderType::FRAGMENT);
  // attach the source
  shader->loadShaderSource(PointVertex,"shaders/PointVertex.glsl");
  shader->loadShaderSource(PointFragment,"shaders/PointFragment.glsl");
  // compile the shaders
  shader->compileShader(PointVertex);
  shader->compileShader(PointFragment);
  // add them to the program
  shader->attachShaderToProgram(Point,PointVertex);
  shader->attachShaderToProgram(Point,PointFragment);
  // now we have associated this data we can link the shader
  shader->linkProgramObject(Point);
  // and make it active ready to load values
  (*shader)["Point"]->use();


  // the shader will use the currently active material and light0 so set them
  ngl::Material m( ngl::STDMAT::GOLD );
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader( "material" );
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from( 10, 10, 10 );
  ngl::Vec3 to( 0, 0, 0 );
  ngl::Vec3 up( 0, 1, 0 );
  // now load to our new camera
  m_cam.set( from, to, up );
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam.setShape( 45.0f, 720.0f / 576.0f, 0.05f, 350.0f );
  shader->setUniform( "viewerPos", m_cam.getEye().toVec3() );
  // now create our light that is done after the camera so we can pass the
  // transpose of the projection matrix to the light to do correct eye space
  // transformations
  ngl::Mat4 iv = m_cam.getViewMatrix();
  iv.transpose();
  ngl::Light light( ngl::Vec3( -2, 5, 2 ), ngl::Colour( 1, 1, 1, 1 ), ngl::Colour( 1, 1, 1, 1 ), ngl::LightModes::POINTLIGHT );
  light.setTransform( iv );
  // load these values to the shader as well
  light.loadToShader( "light" );


  m_text.reset( new ngl::Text(QFont("Arial",14)));
  m_text->setScreenSize(width(),height());

  m_particleTimer=startTimer(10);

  m_fps=0;
  m_frames=0;
  m_timer.start();

//  m_particles.reserve(_numParticles*sizeof(Particle*));
//  GLm_particles.reserve(_numParticles*sizeof(GLParticle*));


  m_particles.reset(  new Particle[_numParticles]);
  GLm_particles.reset( new GLParticle[_numParticles]);

  ngl::Random *rng=ngl::Random::instance();

  for (int i = 0; i < _numParticles; ++i)
  {
     Particle p;
     GLParticle g;
     g.px=p.px=i;
     g.py=p.py=0;
     g.pz=p.pz=0;
     ngl::Colour c=rng->getRandomColour();
     p.m_r=g.pr=c.m_r;
     p.m_g=g.pg=c.m_g;
     p.m_b=g.pb=c.m_b;

//     m_particles.push_back(std::unique_ptr<Particle>(new Particle(ngl::Vec3(i,0,0),ngl::Vec3(1,1,1))));
//     GLm_particles.push_back(std::unique_ptr<GLParticle>(new GLParticle()));
     m_particles[i]=p;
     GLm_particles[i]=g;
  }






  m_vao.reset( ngl::VAOFactory::createVAO(ngl::simpleVAO,GL_POINTS));


  m_vao->bind();
  // now copy the data
  m_vao->setData(ngl::SimpleVAO::VertexData(_numParticles*sizeof(GLParticle),GLm_particles[0].px));
  m_vao->setVertexAttributePointer(0,3,GL_FLOAT,sizeof(GLParticle),0);
  m_vao->setVertexAttributePointer(1,3,GL_FLOAT,sizeof(GLParticle),3);
  m_vao->setNumIndices(_numParticles);
  m_vao->unbind();

  ngl::VAOPrimitives* prim = ngl::VAOPrimitives::instance();
  prim->createSphere("sphere",0.1,10);



}

void NGLScene::updateParticles()
{
//    int concurentThreadsSupported = std::thread::hardware_concurrency();

//    #pragma omp parallel num_threads(8)
//    {

//        #pragma omp for schedule(dynamic, 8)
//        for (int i = 0; i < _numParticles; ++i)
//        {
//            ngl::Vec3 dir=rng->getRandomVec3();
//            dir *=0.05;
//           m_particles[i]->vel.set(dir);
//           m_particles[i]->pos.set(m_particles[i]->pos + m_particles[i]->vel) ;
//        }
//    }




//    tbb::parallel_for( 0, _numParticles,
//        [&](int i)
//        {
//            ngl::Vec3 dir=rng->getRandomVec3();
//            dir *=0.05;
//           m_particles[i]->vel.set(dir);
//           m_particles[i]->pos.set(m_particles[i]->pos /*+ m_particles[i]->vel*/) ;
//        }
//    );



    //map buffer vao to GPU once

    m_vao->bind();
    ngl::Real *glPtr=m_vao->mapBuffer();
    unsigned int glIndex=0;

    ngl::Random *rng=ngl::Random::instance();

    for (int i = 0; i < _numParticles; ++i)
    {
        ngl::Vec3 dir=rng->getRandomVec3();
        dir *=2;
       //update particles
//       m_particles[i]->vel.set(dir);
//       m_particles[i]->pos.set(m_particles[i]->pos /*+ m_particles[i]->vel*/) ;

        m_particles[i].m_dx=dir.m_x;
        m_particles[i].m_dy=dir.m_y;
        m_particles[i].m_dz=dir.m_z;


        m_particles[i].px += m_particles[i].m_dx/100;
        m_particles[i].py += m_particles[i].m_dy/100;
        m_particles[i].pz += m_particles[i].m_dz/100;


       //update mapped pointer
//       glPtr[glIndex]=m_particles[i]->pos.m_x;
//       glPtr[glIndex+1]=m_particles[i]->pos.m_y;
//       glPtr[glIndex+2]=m_particles[i]->pos.m_z;
       glPtr[glIndex]=m_particles[i].px;
       glPtr[glIndex+1]=m_particles[i].py;
       glPtr[glIndex+2]=m_particles[i].pz;
       ngl::Colour c(1,0,0,1);
       glPtr[glIndex+3]=c.m_r;
       glPtr[glIndex+4]=c.m_g;
       glPtr[glIndex+5]=c.m_b;

       glIndex+=6;

    }


  m_vao->unmapBuffer();

  m_vao->unbind();



}
#include <ngl/Transformation.h>
void NGLScene::loadMatricesToShader(int particleNumber)
{
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;

  ngl::Transformation trans;
//  trans.setPosition(m_particles[particleNumber]->pos);


  M            = m_mouseGlobalTX*trans.getMatrix();
  MV           = m_cam.getViewMatrix() * M;
  MVP          = m_cam.getVPMatrix() * M;

  normalMatrix = MV;
  normalMatrix.inverse().transpose();
  shader->setUniform( "MV", MV );
  shader->setUniform( "MVP", MVP );
  shader->setUniform( "normalMatrix", normalMatrix );
  shader->setUniform( "M", M );


}

void NGLScene::paintGL()
{
  glViewport( 0, 0, m_win.width, m_win.height );
  // clear the screen and depth buffer
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  ++m_frames;


  // grab an instance of the shader manager
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();
  //( *shader )[ "Phong" ]->use();
  ( *shader )[ "Point" ]->use();

  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX( m_win.spinXFace );
  rotY.rotateY( m_win.spinYFace );
  // multiply the rotations
  m_mouseGlobalTX = rotX * rotY;
  // add the translations
  m_mouseGlobalTX.m_m[ 3 ][ 0 ] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[ 3 ][ 1 ] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[ 3 ][ 2 ] = m_modelPos.m_z;

  // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives* prim = ngl::VAOPrimitives::instance();

  // draw
//  int concurentThreadsSupported = std::thread::hardware_concurrency();


  double start = omp_get_wtime( );

    // do your code here

    //this drawing is unsynchronized with QT framework (flcikering effect! beware!!)
//  #pragma omp parallel num_threads(8)
//  {
//      #pragma omp for schedule(dynamic, 8)

//      for (int i = 0; i < _numParticles; ++i)
//      {
//         loadMatricesToShader(i);
//         prim->draw( "sphere" );

//      }
//  }


//        for (int i = 0; i < _numParticles; ++i)
//        {
//           loadMatricesToShader(i);
//           prim->draw( "sphere" );

//        }

  //this drawing is unsynchronized with QT framework (flcikering effect! beware!!)
//    tbb::parallel_for( 0, _numParticles,
//        [&](int i)
//        {
//           loadMatricesToShader(i);
//           prim->draw( "sphere" );
//        }
//    );



  //faster drawing one vao instead of individual sphere down the GPU bus



  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat4 M;
  ngl::Transformation trans;
  trans.setPosition(ngl::Vec3(0,0,0));

  M=trans.getMatrix()*m_mouseGlobalTX;
  MV=m_cam/*m_emitter->getCam()*/.getViewMatrix()*M;
  MVP=m_cam/*m_emitter->getCam()*/.getProjectionMatrix()*MV;
  shader->setUniform("MVP",MVP/*vp*_rot*/);

  m_vao->bind();
  m_vao->draw();
  m_vao->unbind();




  double end = omp_get_wtime( );
  double wtick = omp_get_wtick( );

//  printf_s("start = %.16g\nend = %.16g\ndiff = %.16g\n",
//           start, end, end - start);

//  printf_s("wtick = %.16g\n1/wtick = %.16g\n",
//           wtick, 1.0 / wtick);



  m_text->setColour(1,1,1);
  //QString text=QString("Wind Vector  %1 %2 %3").arg(m_wind.m_x).arg(m_wind.m_y).arg(m_wind.m_z);
  QString text=QString("%2 fps").arg(m_fps);
  m_text->renderText(10,20,text);


  text=QString("%5 delta").arg(end - start);
  m_text->renderText(10,40,text);


  //glPointSize(1.0);
  glEnable(GL_PROGRAM_POINT_SIZE);
  // Enable blending
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}


void NGLScene::timerEvent(QTimerEvent *_event )
{
    if(_event->timerId() ==   m_particleTimer)
    {
        if( m_timer.elapsed() > 1000.0)
        {
          m_fps=m_frames;
          m_frames=0;
          m_timer.restart();
        }

    }
    updateParticles();

    update();


        // re-draw GL
}


//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent( QKeyEvent* _event )
{
  // that method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch ( _event->key() )
  {
    // escape key to quit
    case Qt::Key_Escape:
      QGuiApplication::exit( EXIT_SUCCESS );
      break;
// turn on wirframe rendering
#ifndef USINGIOS_
    case Qt::Key_W:
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      break;
    // turn off wire frame
    case Qt::Key_S:
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      break;
#endif
    // show full screen
    case Qt::Key_F:
      showFullScreen();
      break;
    // show windowed
    case Qt::Key_N:
      showNormal();
      break;
    case Qt::Key_Space :
      m_win.spinXFace=0;
      m_win.spinYFace=0;
      m_modelPos.set(ngl::Vec3::zero());
    break;
    default:
      break;
  }
  update();
}
