#pragma once

#include <vector>
#include "scene.h"

void InitDataContainer(GuiDataContainer* guiData);
void pathtraceInit(Scene *scene);
void pathtraceFree(Scene* scene);
void pathtrace(uchar4 *pbo, int frame, int iteration);
void showGBuffer(uchar4* pbo, int renderSelect);
void showImage(uchar4* pbo, int iter);