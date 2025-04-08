#include "Anime.h"
#include "Timer.h"


void Anime::LoadVertexBuffer() {
    // clear vertex buffers
    vertex_buffers.clear();
    // load vertex buffers
    for (const auto& model: models){ // for each model
        const std::string& model_name = model.first;
        const Object& obj = model.second;

        vertex_buffers[model_name] = std::vector<MyVertexShader::Input>();
        for (const Vertex& vertex: *obj.vertices){ // for each vertex in the model
            MyVertexShader::Input vs_input;
            vs_input.model_pos = vertex.position;
            vs_input.model_normal = vertex.normal;
            vs_input.texcoord = vertex.texcoord;
            vertex_buffers[model_name].emplace_back(vs_input);
        }
    }
}

std::unique_ptr<Engine> Anime::InitEngine(int render_thread_num, int blend_thread_num,
                                          const glm::vec3& bg_color, float bg_depth) {
    auto engine = std::make_unique<Engine>(width, height, render_thread_num, blend_thread_num, bg_color, bg_depth, true);

    //////// set vertex-shader parameters
    auto vshader = std::make_unique<MyVertexShader>();

    // initialize model transform
    glm::vec3 translation(0.0f, 0.0f, 0.0f);
    float rotation = 0.0f;
    float scale = 1.0f;
    vshader->model = GetModelTransform(translation, rotation, scale);

    // initialize view transform
    const glm::vec3 eye(0.0f, 0.0f, 7.0f); // camera position
    const glm::vec3 target(0.0f, 0.0f, 0.0f); // `eye` and `target` defines the direction the camera looking at
    const glm::vec3 up(0.0f, 1.0f, 0.0f); // `up` vector of the camera
    vshader->view = GetViewTransform(eye, target, up);

    // initialize perspective projection transform
    glm::vec3 light_pos(0.0f, 0.0f, 10.0f);
    constexpr float fovy = glm::radians(60.0f); // field of view
    const float aspect = 1.0f * width / height; // aspect of the window (width / height)
    const float znear = 0.01f; // near plane for clipping
    const float zfar = 100.0f; // far plane for clipping
    vshader->projection = GetPerspectiveProjectionTransform(fovy, aspect, znear, zfar);

    // initialize orthographic projection transform
    // glm::vec3 light_pos(0.0f, 0.0f, 10000.0f);
    // const float orth_width = 7.5f;
    // const float orth_height = orth_width / width * height;
    // const float znear = 0.1f; // near plane for clipping
    // const float zfar = 100.0f; // far plane for clipping
    // vshader->projection = GetOrthographicProjectionTransform(orth_width, orth_height, znear, zfar);

    for (const auto& nvb: vertex_buffers) {
        const std::string& model_name = nvb.first;
        const Object& obj = models[model_name];
        const std::vector<MyVertexShader::Input>& vertex_buffer = nvb.second;

        //////// set fragment-shader parameters
        auto fshader = std::make_unique<MyFragmentShader>();
        fshader->light_pos = light_pos;
        fshader->texture = obj.texture.get();
        
        //////// load shaders
        engine->pipeline_manager->CreatePipeline(vertex_buffer, vshader.get(), fshader.get(), true);
        fshaders.emplace_back(std::move(fshader));
    }
    vshaders.emplace_back(std::move(vshader));

    return engine;
}

std::unique_ptr<Anime::MyPipeline> Anime::InitPipeline(){
    //////// set vertex-shader parameters
    auto vshader = std::make_unique<MyVertexShader>();

    // initialize model transform
    glm::vec3 translation(0.0f, 0.0f, 0.0f);
    float rotation = 0.0f;
    float scale = 1.5f;
    vshader->model = GetModelTransform(translation, rotation, scale);

    // initialize view transform
    const glm::vec3 eye(0.0f, 0.0f, 10.0f); // camera position
    const glm::vec3 target(0.0f, 0.0f, 0.0f); // `eye` and `target` defines the direction the camera looking at
    const glm::vec3 up(0.0f, 1.0f, 0.0f); // `up` vector of the camera
    vshader->view = GetViewTransform(eye, target, up);

    // initialize projection transform
    glm::vec3 light_pos(0.0f, 0.0f, 10.0f);
    constexpr float fovy = glm::radians(60.0f); // field of view
    const float aspect = 1.0f * width / height; // aspect of the window (width / height)
    const float znear = 0.01f; // near plane for clipping
    const float zfar = 100.0f; // far plane for clipping
    vshader->projection = GetPerspectiveProjectionTransform(fovy, aspect, znear, zfar);

    // const float orth_width = 15.0f;
    // glm::vec3 light_pos(0.0f, 0.0f, 10000.0f);
    // const float orth_height = orth_width / width * height;
    // const float znear = 0.1f; // near plane for clipping
    // const float zfar = 100.0f; // far plane for clipping
    // vshader->projection = GetOrthographicProjectionTransform(orth_width, orth_height, znear, zfar);

    //////// set fragment-shader parameters
    auto fshader = std::make_unique<MyFragmentShader>();
    fshader->light_pos = light_pos;
    fshader->light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    
    //////// load shaders
    std::unique_ptr<MyPipeline> pipeline = std::make_unique<MyPipeline>(vshader.get(), fshader.get());
    vshaders.emplace_back(std::move(vshader));
    fshaders.emplace_back(std::move(fshader));
    return pipeline;
}

