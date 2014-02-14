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

#include "hair_loader.h"

#define CONVERT_TO_BINARY 0

namespace embree
{
  const int hair_bin_magick = 0x12EF3F90;

  int loadHairASCII(const FileName& fileName, OBJScene::HairSet* hairset, Vec3fa& offset)
  {  
    /* open hair file */
    FILE* f = fopen(fileName.c_str(),"r");
    if (!f) throw std::runtime_error("could not open " + fileName.str());

    char line[10000];
    fgets(line,10000,f);
    int numCurves = 0;
    
    while (fgets(line,10000,f) && !feof(f))
    {
      /* comment */
      if (line[0] == '#')
	continue;
      
      if (!strncmp(line,"Curve:",strlen("Curve:")))
      {
        char name[1000];
        unsigned int tracks, points;
        sscanf(line,"Curve: %s %d Tracks %d Points",name,&tracks,&points);

        /* skip Tracks line */
        fgets(line,10000,f);
        
        const int vertex_start_id = hairset->v.size();
        
        double x,y,z,w;
        unsigned int id = 0;
        for (int i=0; i<points; i++)
        {
          fgets(line,10000,f);

          /* comment */
          if (line[0] == '#' || !strncmp(line," Tracks:",strlen(" Tracks:")))
            continue;

          Vec3fa v;
          if (i == 0) sscanf(line,"%d : Bezier %f %f %f %f",&id,&v.x,&v.y,&v.z,&v.w);
          else        sscanf(line,"%d : %f %f %f %f",&id,&v.x,&v.y,&v.z,&v.w);
          //printf("%d %d : %f %f %f %f \n",id,vertex_start_id+id,v.x,v.y,v.z,v.w);		
          v.x-=offset.x;
          v.y-=offset.y;
          v.z-=offset.z;
          hairset->v.push_back(v);
        }
        
        /* add indices to hair starts */
        for (int i=0; i<points-1; i+=3)
          hairset->hairs.push_back(OBJScene::Hair(vertex_start_id + i,numCurves));
	
        if (id != points-1) 
          throw std::runtime_error("hair parsing error");

        numCurves++;
      }
    }
    fclose(f);
    return numCurves;
  }

  int loadHairBin(const FileName& fileName, OBJScene::HairSet* hairset, Vec3fa& offset)
  {  
    FILE* fin = fopen(fileName.c_str(),"rb");
    if (!fin) throw std::runtime_error("could not open " + fileName.str());
    int magick; fread(&magick,sizeof(int),1,fin);
    if (magick != hair_bin_magick)
      throw std::runtime_error("invalid binary hair file " + fileName.str());
    int numHairs; fread(&numHairs,sizeof(int),1,fin);
    int numPoints; fread(&numPoints,sizeof(int),1,fin);
    int numSegments; fread(&numSegments,sizeof(int),1,fin);
    hairset->v.resize(numPoints);
    hairset->hairs.resize(numSegments);
    if (numPoints) fread(&hairset->v[0],sizeof(Vec3fa),numPoints,fin);
    if (numSegments) fread(&hairset->hairs[0],sizeof(OBJScene::Hair),numSegments,fin);
    fclose(fin);

    for (size_t i=0; i<numPoints; i++) {
      hairset->v[i].x-=offset.x;
      hairset->v[i].y-=offset.y;
      hairset->v[i].z-=offset.z;
    }
    return numHairs;
  }

  void loadHair(const FileName& fileName, OBJScene& scene, Vec3fa& offset)
  {
    /* add new hair set to scene */
    OBJScene::HairSet* hairset = new OBJScene::HairSet;
    scene.hairsets.push_back(hairset);
 
#if CONVERT_TO_BINARY   
    offset = Vec3fa(zero);
#endif

    int numHairs = 0;
    if (fileName.ext() == "txt")
      numHairs = loadHairASCII(fileName,hairset,offset);
    else
      numHairs = loadHairBin(fileName,hairset,offset);

#if CONVERT_TO_BINARY
    FILE* fout = fopen(fileName.setExt(".bin").c_str(),"wb");
    if (!fout) throw std::runtime_error("could not open " + fileName.str());
    fwrite(&hair_bin_magick,sizeof(int),1,fout);
    fwrite(&numHairs,sizeof(int),1,fout);
    int numPoints = hairset->v.size();
    int numSegments = hairset->hairs.size();
    fwrite(&numPoints,sizeof(int),1,fout);
    fwrite(&numSegments,sizeof(int),1,fout);
    if (numPoints) fwrite(&hairset->v[0],sizeof(Vec3fa),numPoints,fout);
    if (numSegments) fwrite(&hairset->hairs[0],sizeof(OBJScene::Hair),numSegments,fout);
    fclose(fout);
#endif
  }
}

  