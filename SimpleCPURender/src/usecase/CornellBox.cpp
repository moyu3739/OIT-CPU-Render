#include "CornellBox.h"
#include "Timer.h"


std::unique_ptr<Engine> CornellBox::InitEngine(int render_thread_num, int blend_thread_num,
                                          const glm::vec3& bg_color, float bg_depth){
    auto engine = std::make_unique<Engine>(width, height, render_thread_num, blend_thread_num, bg_color, bg_depth, true);
    global_model = std::make_shared<glm::mat4>(
        GetModelTransform(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 1.0f));

    for (const auto& nvb: vertex_buffers) {
        //////// set vertex-shader parameters
        auto vshader = std::make_unique<MyVertexShader>();

        // initialize model transform
        
        float rotation = 0.0f;
        float scale = 1.0f;
        if (combined) {
            vshader->model = GetModelTransform(glm::vec3(0.0f, 0.0f, 0.0f), rotation, scale);
        }
        else {
            glm::vec3 t_min(-1.5f);
            glm::vec3 t_max(1.5f);
            vshader->model = GetModelTransform(RandomVec3(t_min, t_max), rotation, scale);
        }
        vshader->global_model = global_model;

        // initialize view transform
        const glm::vec3 eye(0.0f, 0.0f, 5.0f); // camera position
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


        const std::string& model_name = nvb.first;
        const Object& obj = models[model_name];
        const std::vector<VertexShader::InputWrapper>& vertex_buffer = nvb.second;

        //////// set fragment-shader parameters
        auto fshader = std::make_unique<MyFragmentShader>();
        fshader->light_pos = light_pos;

        //////// load shaders
        engine->pipeline_manager->CreatePipeline(vertex_buffer, vshader.get(), fshader.get(), true);
        vshaders.emplace_back(std::move(vshader));
        fshaders.emplace_back(std::move(fshader));
    }
    
    return engine;
}

void CornellBox::LoadVertexBufferShapesDivided() {
    for (int i = 0; i < N_shapes; i++) {
        std::string name = "shape " + std::to_string(i);
        std::unique_ptr<Shape> shape;

        glm::vec3 color = RandomVec3(glm::vec3(0.5f), glm::vec3(1.0f));
        switch(RandomInt(0, 3)) {
            case 0: { // sphere
                float r = RandomFloat(0.1f, 0.2f);
                shape = std::make_unique<Sphere>(r, 32, color);
                break;
            }
            case 1: { // cylinder
                float r = RandomFloat(0.1f, 0.2f);
                float h = RandomFloat(0.2f, 0.4f);
                shape = std::make_unique<Cylinder>(r, h, 32, color);
                break;
            }
            case 2: { // box
                glm::vec3 size = RandomVec3(glm::vec3(0.2f), glm::vec3(0.4f));
                shape = std::make_unique<Box>(size, color);
                break;
            }
            case 3: { // cone
                float r = RandomFloat(0.1f, 0.2f);
                float h = RandomFloat(0.2f, 0.4f);
                shape = std::make_unique<Cone>(r, h, 64, color);
                break;
            }
            default:
                assert(false);
        }

        vertex_datas[name] = shape->GetVertexData();
        vertex_buffers[name] = std::vector<VertexShader::InputWrapper>(vertex_datas[name].size());
        auto& vd = vertex_datas[name];
        auto& vb = vertex_buffers[name];
        for (int j = 0; j < vd.size(); j++) {
            vb[j] = VertexShader::InputWrapper{&vd[j]};
        }

        shapes.emplace_back(std::move(shape));
    }
}

void CornellBox::LoadVertexBufferShapesCombined() {
    LoadVertexBufferShapesDivided();

    for (auto& vertex_data: vertex_datas) {
        auto& vertices = vertex_data.second;
        glm::vec3 translation = RandomVec3(glm::vec3(-1.5f), glm::vec3(1.5f));
        for (auto& vertex: vertices) vertex.position += translation;
    }

    std::vector<VertexShader::InputWrapper> vertex_buffers_combined;
    for (auto& vertex_buffer: vertex_buffers) {
        auto& vertices = vertex_buffer.second;
        vertex_buffers_combined.insert(
            vertex_buffers_combined.end(), vertices.begin(), vertices.end());
    }

    vertex_buffers.clear();
    vertex_buffers["combined"] = vertex_buffers_combined;
}
