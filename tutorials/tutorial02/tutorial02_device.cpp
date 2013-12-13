// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "../common/tutorial/tutorial_device.h"

const int numPhi = 5;
const int numTheta = 2*numPhi;

/* render function to use */
renderPixelFunc renderPixel;

// ======================================================================== //
//                         User defined instancing                          //
// ======================================================================== //

struct Instance 
{
  unsigned int geometry;
  RTCScene object;
  int userID;
  AffineSpace3f local2world;
  AffineSpace3f world2local;
  Vec3f lower;
  Vec3f upper;
};

void instanceIntersectFunc(const Instance* instance, RTCRay& ray)
{
  const Vec3f ray_org = ray.org;
  const Vec3f ray_dir = ray.dir;
  const int geomID = ray.geomID;
  ray.org = xfmPoint (instance->world2local,ray_org);
  ray.dir = xfmVector(instance->world2local,ray_dir);
  ray.geomID = -1;
  rtcIntersect(instance->object,ray);
  ray.org = ray_org;
  ray.dir = ray_dir;
  if (ray.geomID == -1) ray.geomID = geomID;
  else ray.instID = instance->userID;
}

void instanceOccludedFunc(const Instance* instance, RTCRay& ray)
{
  const Vec3f ray_org = ray.org;
  const Vec3f ray_dir = ray.dir;
  ray.org = xfmPoint (instance->world2local,ray_org);
  ray.dir = xfmVector(instance->world2local,ray_dir);
  rtcOccluded(instance->object,ray);
  ray.org = ray_org;
  ray.dir = ray_dir;
}

Instance* createInstance (RTCScene scene, RTCScene object, int userID, Vec3f lower, Vec3f upper)
{
  Instance* instance = new Instance;
  instance->object = object;
  instance->userID = userID;
  instance->lower = lower;
  instance->upper = upper;
  instance->local2world.l.vx = Vec3f(1,0,0);
  instance->local2world.l.vy = Vec3f(0,1,0);
  instance->local2world.l.vz = Vec3f(0,0,1);
  instance->local2world.p    = Vec3f(0,0,0);
  instance->geometry = rtcNewUserGeometry(scene);
  rtcSetUserData(scene,instance->geometry,instance);
  rtcSetIntersectFunction(scene,instance->geometry,(RTCIntersectFunc)&instanceIntersectFunc);
  rtcSetOccludedFunction (scene,instance->geometry,(RTCOccludedFunc )&instanceOccludedFunc);
  return instance;
}

void updateInstance (RTCScene scene, Instance* instance)
{
  unsigned int geometry = instance->geometry;
  instance->world2local = rcp(instance->local2world);
  Vec3f l = instance->lower;
  Vec3f u = instance->upper;
  Vec3f p000 = xfmPoint(instance->local2world,Vec3f(l.x,l.y,l.z));
  Vec3f p001 = xfmPoint(instance->local2world,Vec3f(l.x,l.y,u.z));
  Vec3f p010 = xfmPoint(instance->local2world,Vec3f(l.x,u.y,l.z));
  Vec3f p011 = xfmPoint(instance->local2world,Vec3f(l.x,u.y,u.z));
  Vec3f p100 = xfmPoint(instance->local2world,Vec3f(u.x,l.y,l.z));
  Vec3f p101 = xfmPoint(instance->local2world,Vec3f(u.x,l.y,u.z));
  Vec3f p110 = xfmPoint(instance->local2world,Vec3f(u.x,u.y,l.z));
  Vec3f p111 = xfmPoint(instance->local2world,Vec3f(u.x,u.y,u.z));
  Vec3f lower = min(min(min(p000,p001),min(p010,p011)),min(min(p100,p101),min(p110,p111)));
  Vec3f upper = max(max(max(p000,p001),max(p010,p011)),max(max(p100,p101),max(p110,p111)));
  rtcSetBounds(scene,instance->geometry,lower.x,lower.y,lower.z,upper.x,upper.y,upper.z);
  rtcUpdate(scene,instance->geometry);
}

// ======================================================================== //
//                     User defined sphere geometry                         //
// ======================================================================== //

struct Sphere
{
  Vec3f p;                      //!< position of the sphere
  float r;                      //!< radius of the sphere
  unsigned int geometry; 
};

void sphereIntersectFunc(const Sphere* sphere, RTCRay& ray)
{
  const Vec3f v = sub(ray.org,sphere->p);
  const float A = dot(ray.dir,ray.dir);
  const float B = 2.0f*dot(v,ray.dir);
  const float C = dot(v,v) - sqr(sphere->r);
  const float D = B*B - 4.0f*A*C;
  if (D < 0.0f) return;
  const float Q = sqrt(D);
  const float rcpA = rcp(A);
  const float t0 = 0.5f*rcpA*(-B-Q);
  const float t1 = 0.5f*rcpA*(-B+Q);
  if (ray.tnear < t0 & t0 < ray.tfar) {
    ray.u = 0.0f;
    ray.v = 0.0f;
    ray.tfar = t0;
    ray.geomID = sphere->geometry;
    ray.primID = 0;
    ray.Ng = sub(add(ray.org,mul(t0,ray.dir)),sphere->p);
  }
  if (ray.tnear < t1 & t1 < ray.tfar) {
    ray.u = 0.0f;
    ray.v = 0.0f;
    ray.tfar = t1;
    ray.geomID = sphere->geometry;
    ray.primID = 0;
    ray.Ng = sub(add(ray.org,mul(t1,ray.dir)),sphere->p);
  }
}

void sphereOccludedFunc(const Sphere* sphere, RTCRay& ray)
{
  const Vec3f v = sub(ray.org,sphere->p);
  const float A = dot(ray.dir,ray.dir);
  const float B = 2.0f*dot(v,ray.dir);
  const float C = dot(v,v) - sqr(sphere->r);
  const float D = B*B - 4.0f*A*C;
  if (D < 0.0f) return;
  const float Q = sqrt(D);
  const float rcpA = rcp(A);
  const float t0 = 0.5f*rcpA*(-B-Q);
  const float t1 = 0.5f*rcpA*(-B+Q);
  if (ray.tnear < t0 & t0 < ray.tfar) {
    ray.geomID = 0;
  }
  if (ray.tnear < t1 & t1 < ray.tfar) {
    ray.geomID = 0;
  }
}

Sphere* createAnalyticalSphere (RTCScene scene, Vec3f p, float r)
{
  Sphere* sphere = new Sphere;
  sphere->p = p;
  sphere->r = r;
  sphere->geometry = rtcNewUserGeometry(scene);
  rtcSetBounds(scene,sphere->geometry,
               sphere->p.x-sphere->r,sphere->p.y-sphere->r,sphere->p.z-sphere->r,
               sphere->p.x+sphere->r,sphere->p.y+sphere->r,sphere->p.z+sphere->r);
  rtcSetUserData(scene,sphere->geometry,sphere);
  rtcSetIntersectFunction(scene,sphere->geometry,(RTCIntersectFunc)&sphereIntersectFunc);
  rtcSetOccludedFunction (scene,sphere->geometry,(RTCOccludedFunc )&sphereOccludedFunc);
  return sphere;
}

// ======================================================================== //
//                      Triangular sphere geometry                          //
// ======================================================================== //

unsigned int createTriangulatedSphere (RTCScene scene, Vec3f p, float r)
{
  /* create triangle mesh */
  unsigned int mesh = rtcNewTriangleMesh (scene, RTC_GEOMETRY_STATIC, 2*numTheta*(numPhi-1), numTheta*(numPhi+1));
  
  /* map triangle and vertex buffers */
  Vertex* vertices = (Vertex*) rtcMapBuffer(scene,mesh,RTC_VERTEX_BUFFER); 
  Triangle* triangles = (Triangle*) rtcMapBuffer(scene,mesh,RTC_INDEX_BUFFER);

  /* create sphere */
  int tri = 0;
  const float rcpNumTheta = rcp((float)numTheta);
  const float rcpNumPhi   = rcp((float)numPhi);
  for (int phi=0; phi<=numPhi; phi++)
  {
    for (int theta=0; theta<numTheta; theta++)
    {
      const float phif   = phi*float(pi)*rcpNumPhi;
      const float thetaf = theta*2.0f*float(pi)*rcpNumTheta;

      Vertex& v = vertices[phi*numTheta+theta];
      v.x = p.x + r*sin(phif)*sin(thetaf);
      v.y = p.y + r*cos(phif);
      v.z = p.z + r*sin(phif)*cos(thetaf);
    }
    if (phi == 0) continue;

    for (int theta=1; theta<=numTheta; theta++) 
    {
      int p00 = (phi-1)*numTheta+theta-1;
      int p01 = (phi-1)*numTheta+theta%numTheta;
      int p10 = phi*numTheta+theta-1;
      int p11 = phi*numTheta+theta%numTheta;

      if (phi > 1) {
        triangles[tri].v0 = p10; 
        triangles[tri].v1 = p00; 
        triangles[tri].v2 = p01; 
        tri++;
      }

      if (phi < numPhi) {
        triangles[tri].v0 = p11; 
        triangles[tri].v1 = p10;
        triangles[tri].v2 = p01;
        tri++;
      }
    }
  }
  rtcUnmapBuffer(scene,mesh,RTC_VERTEX_BUFFER); 
  rtcUnmapBuffer(scene,mesh,RTC_INDEX_BUFFER);
  return mesh;
}

/* creates a ground plane */
unsigned int createGroundPlane (RTCScene scene)
{
  /* create a triangulated plane with 2 triangles and 4 vertices */
  unsigned int mesh = rtcNewTriangleMesh (scene, RTC_GEOMETRY_STATIC, 2, 4);

  /* set vertices */
  Vertex* vertices = (Vertex*) rtcMapBuffer(scene,mesh,RTC_VERTEX_BUFFER); 
  vertices[0].x = -10; vertices[0].y = -2; vertices[0].z = -10; 
  vertices[1].x = -10; vertices[1].y = -2; vertices[1].z = +10; 
  vertices[2].x = +10; vertices[2].y = -2; vertices[2].z = -10; 
  vertices[3].x = +10; vertices[3].y = -2; vertices[3].z = +10;
  rtcUnmapBuffer(scene,mesh,RTC_VERTEX_BUFFER); 

  /* set triangles */
  Triangle* triangles = (Triangle*) rtcMapBuffer(scene,mesh,RTC_INDEX_BUFFER);
  triangles[0].v0 = 0; triangles[0].v1 = 2; triangles[0].v2 = 1;
  triangles[1].v0 = 1; triangles[1].v1 = 2; triangles[1].v2 = 3;
  rtcUnmapBuffer(scene,mesh,RTC_INDEX_BUFFER);

  return mesh;
}

/* scene data */
RTCScene g_scene  = NULL;
RTCScene g_scene0 = NULL;
RTCScene g_scene1 = NULL;
RTCScene g_scene2 = NULL;

Instance* g_instance0 = NULL;
Instance* g_instance1 = NULL;
Instance* g_instance2 = NULL;
Instance* g_instance3 = NULL;

Vec3f colors[5][4];

/* called by the C++ code for initialization */
extern "C" void device_init (int8* cfg)
{
  /* initialize ray tracing core */
  rtcInit(cfg);

  /* create scene */
  g_scene = rtcNewScene(RTC_SCENE_DYNAMIC,RTC_INTERSECT1);

  /* create scene with 4 analytical spheres */
  g_scene0 = rtcNewScene(RTC_SCENE_STATIC,RTC_INTERSECT1);
  createAnalyticalSphere(g_scene0,Vec3f( 0, 0,+1),0.5);
  createAnalyticalSphere(g_scene0,Vec3f(+1, 0, 0),0.5);
  createAnalyticalSphere(g_scene0,Vec3f( 0, 0,-1),0.5);
  createAnalyticalSphere(g_scene0,Vec3f(-1, 0, 0),0.5);
  rtcCommit(g_scene0);

  /* create scene with 4 triangulated spheres */
  g_scene1 = rtcNewScene(RTC_SCENE_STATIC,RTC_INTERSECT1);
  createTriangulatedSphere(g_scene1,Vec3f( 0, 0,+1),0.5);
  createTriangulatedSphere(g_scene1,Vec3f(+1, 0, 0),0.5);
  createTriangulatedSphere(g_scene1,Vec3f( 0, 0,-1),0.5);
  createTriangulatedSphere(g_scene1,Vec3f(-1, 0, 0),0.5);
  rtcCommit(g_scene1);

  /* create scene with 2 triangulated and 2 analytical spheres */
  g_scene2 = rtcNewScene(RTC_SCENE_STATIC,RTC_INTERSECT1);
  createTriangulatedSphere(g_scene2,Vec3f( 0, 0,+1),0.5);
  createAnalyticalSphere  (g_scene2,Vec3f(+1, 0, 0),0.5);
  createTriangulatedSphere(g_scene2,Vec3f( 0, 0,-1),0.5);
  createAnalyticalSphere  (g_scene2,Vec3f(-1, 0, 0),0.5);
  rtcCommit(g_scene2);

  /* instantiate geometry */
  createGroundPlane(g_scene);
  g_instance0 = createInstance(g_scene,g_scene0,0,Vec3f(-2,-2,-2),Vec3f(+2,+2,+2));
  g_instance1 = createInstance(g_scene,g_scene1,1,Vec3f(-2,-2,-2),Vec3f(+2,+2,+2));
  g_instance2 = createInstance(g_scene,g_scene2,2,Vec3f(-2,-2,-2),Vec3f(+2,+2,+2));
  g_instance3 = createInstance(g_scene,g_scene2,3,Vec3f(-2,-2,-2),Vec3f(+2,+2,+2));

  /* set all colors */
  colors[0][0] = Vec3f(0.25,0,0);
  colors[0][1] = Vec3f(0.50,0,0);
  colors[0][2] = Vec3f(0.75,0,0);
  colors[0][3] = Vec3f(1.00,0,0);

  colors[1][0] = Vec3f(0,0.25,0);
  colors[1][1] = Vec3f(0,0.50,0);
  colors[1][2] = Vec3f(0,0.75,0);
  colors[1][3] = Vec3f(0,1.00,0);

  colors[2][0] = Vec3f(0,0,0.25);
  colors[2][1] = Vec3f(0,0,0.50);
  colors[2][2] = Vec3f(0,0,0.75);
  colors[2][3] = Vec3f(0,0,1.00);

  colors[3][0] = Vec3f(0.25,0.25,0);
  colors[3][1] = Vec3f(0.50,0.50,0);
  colors[3][2] = Vec3f(0.75,0.75,0);
  colors[3][3] = Vec3f(1.00,1.00,0);

  colors[4][0] = Vec3f(1.0,1.0,1.0);
  colors[4][1] = Vec3f(1.0,1.0,1.0);
  colors[4][2] = Vec3f(1.0,1.0,1.0);
  colors[4][3] = Vec3f(1.0,1.0,1.0);

  /* set start render mode */
  renderPixel = renderPixelStandard;
}

/* task that renders a single screen tile */
Vec3fa renderPixelStandard(int x, int y, const Vec3fa& vx, const Vec3fa& vy, const Vec3fa& vz, const Vec3fa& p)
{
  /* initialize ray */
  RTCRay ray;
  ray.org = p;
  ray.dir = normalize(add(mul(x,vx), mul(y,vy), vz));
  ray.tnear = 0.0f;
  ray.tfar = inf;
  ray.geomID = -1;
  ray.primID = -1;
  ray.instID = 4; // set default instance ID
  ray.mask = -1;
  ray.time = 0;
  
  /* intersect ray with scene */
  rtcIntersect(g_scene,ray);
  
  /* shade pixels */
  Vec3f color = Vec3f(0.0f);
  if (ray.geomID != -1) 
  {
    Vec3f diffuse = colors[ray.instID][ray.geomID];
    color = add(color,mul(diffuse,0.5));
    Vec3f lightDir = normalize(Vec3f(-1,-1,-1));
    
    /* initialize shadow ray */
    RTCRay shadow;
    shadow.org = add(ray.org,mul(ray.tfar,ray.dir));
    shadow.dir = neg(lightDir);
    shadow.tnear = 0.001f;
    shadow.tfar = inf;
    shadow.geomID = 1;
    shadow.primID = 0;
    shadow.mask = -1;
    shadow.time = 0;
    
    /* trace shadow ray */
    rtcOccluded(g_scene,shadow);
    
    /* add light contribution */
    if (shadow.geomID)
      color = add(color,mul(diffuse,clamp(-dot(lightDir,normalize(ray.Ng)),0.0f,1.0f)));
  }
  return color;
}

/* task that renders a single screen tile */
void renderTile(int taskIndex, int* pixels,
                     const int width,
                     const int height, 
                     const float time,
                     const Vec3f& vx, 
                     const Vec3f& vy, 
                     const Vec3f& vz, 
                     const Vec3f& p,
                     const int numTilesX, 
                     const int numTilesY)
{
  const int tileY = taskIndex / numTilesX;
  const int tileX = taskIndex - tileY * numTilesX;
  const int x0 = tileX * TILE_SIZE_X;
  const int x1 = min(x0+TILE_SIZE_X,width);
  const int y0 = tileY * TILE_SIZE_Y;
  const int y1 = min(y0+TILE_SIZE_Y,height);

  for (int y = y0; y<y1; y++) for (int x = x0; x<x1; x++)
  {
    /* calculate pixel color */
    Vec3f color = renderPixel(x,y,vx,vy,vz,p);

    /* write color to framebuffer */
    unsigned int r = (unsigned int) (255.0f * clamp(color.x,0.0f,1.0f));
    unsigned int g = (unsigned int) (255.0f * clamp(color.y,0.0f,1.0f));
    unsigned int b = (unsigned int) (255.0f * clamp(color.z,0.0f,1.0f));
    pixels[y*width+x] = (b << 16) + (g << 8) + r;
  }
}

/* called by the C++ code to render */
extern "C" void device_render (int* pixels,
                           const int width,
                           const int height, 
                           const float time,
                           const Vec3f& vx, 
                           const Vec3f& vy, 
                           const Vec3f& vz, 
                           const Vec3f& p)
{
  /* move instances */
  float t = 0.7f*time;
  g_instance0->local2world.p = mul(2.0f,Vec3f(+cos(t),0.0f,+sin(t)));
  g_instance1->local2world.p = mul(2.0f,Vec3f(-cos(t),0.0f,-sin(t)));
  g_instance2->local2world.p = mul(2.0f,Vec3f(-sin(t),0.0f,+cos(t)));
  g_instance3->local2world.p = mul(2.0f,Vec3f(+sin(t),0.0f,-cos(t)));
  updateInstance(g_scene,g_instance0);
  updateInstance(g_scene,g_instance1);
  updateInstance(g_scene,g_instance2);
  updateInstance(g_scene,g_instance3);
  rtcCommit (g_scene);

  /* render all pixels */
  const int numTilesX = (width +TILE_SIZE_X-1)/TILE_SIZE_X;
  const int numTilesY = (height+TILE_SIZE_Y-1)/TILE_SIZE_Y;
  launch_renderTile(numTilesX*numTilesY,pixels,width,height,time,vx,vy,vz,p,numTilesX,numTilesY); 
  rtcDebug();
}

/* called by the C++ code for cleanup */
extern "C" void device_cleanup ()
{
  rtcDeleteScene (g_scene);
  rtcDeleteScene (g_scene0);
  rtcDeleteScene (g_scene1);
  rtcDeleteScene (g_scene2);
  rtcExit();
}