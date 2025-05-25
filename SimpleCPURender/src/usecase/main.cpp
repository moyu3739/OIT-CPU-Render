#include <iostream>
#include <thread>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Application.h"
#include "Anime.h"
#include "Intensity.h"
#include "CornellBox.h"
#include "Phong.h"


int main() {
    Anime anime = Anime(800, 800);
    anime.Run();

    // const float a = 1.72f;
    // const float h = 1.14f;
    // glm::vec3 t_min(-a, -h, -a);
    // glm::vec3 t_max(a, h, a);
    // CornellBox cornell = CornellBox(1200, 800, 200, false, t_min, t_max, 1919810, SHAPE_BOX);
    // cornell.Run();

    // Intensity intensity = Intensity(800, 800);
    // intensity.Run();

    // QuickStart();

    return 0;
}