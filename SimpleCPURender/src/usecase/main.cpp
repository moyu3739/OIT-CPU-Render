#include <iostream>
#include <thread>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Application.h"
#include "Anime.h"
#include "Intensity.h"
#include "CornellBox.h"

#include "Pipeline.h"
#include "Frontend.h"
#include "FrameBuffer.h"
#include "FrameBufferManager.h"
#include "Timer.h"
#include "Engine.h"

#include "Phong.h"


int main() {
    Anime anime = Anime(800, 800);
    anime.Run();

    // CornellBox cornell = CornellBox(800, 800, 200, true, 1919810);
    // cornell.Run();

    // Intensity intensity = Intensity(800, 800);
    // intensity.Run();

    // QuickStart();

    return 0;
}