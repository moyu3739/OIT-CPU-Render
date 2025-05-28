#include <iostream>
#include <thread>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "usecase/Application.h"
#include "usecase/Anime.h"
#include "usecase/Intensity.h"
#include "usecase/CornellBox.h"
#include "usecase/Phong.h"

#include "test/TestAllocator.h"
#include "test/TestList.h"


void UnitTest() {
    TestAllocator();
    TestList();
}

int main() {
    // UnitTest();

    Anime anime = Anime(800, 800);
    anime.Run();

    // const float a = 1.5f;
    // const float h = 1.5f;
    // glm::vec3 t_min(-a, -h, -a);
    // glm::vec3 t_max(a, h, a);
    // CornellBox cornell = CornellBox(800, 800, 200, false, t_min, t_max, 1919810, SHAPE_RANDOM, true, true, 0.95f);
    // cornell.Run();

    // Intensity intensity = Intensity(800, 800, true, true, 0.95f);
    // intensity.Run();

    // QuickStart();

    return 0;
}