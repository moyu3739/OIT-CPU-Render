#include "CornellBox.h"
#include "Timer.h"


std::unique_ptr<Engine> CornellBox::InitEngine(
    int render_thread_num, int blend_thread_num,
    const glm::vec3& bg_color, float bg_depth,
    int pipeline_level, bool enable_oit,
    bool use_backward_pplist, float backward_blend_alpha_threshold
) {
    auto engine = std::make_unique<Engine>(width, height, render_thread_num, blend_thread_num,
        bg_color, bg_depth, pipeline_level, enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
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
            vshader->model = GetModelTransform(random_gen.RandomVec3(t_min, t_max), rotation, scale);
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
        const VertexBuffer& vertex_buffer = nvb.second;

        //////// set fragment-shader parameters
        auto fshader = std::make_unique<MyFragmentShader>();
        fshader->light_pos = light_pos;

        //////// load shaders
        engine->CreatePipeline(vertex_buffer, vshader.get(), fshader.get(), ON_FACE, true);
        vshaders.emplace_back(std::move(vshader));
        fshaders.emplace_back(std::move(fshader));
    }
    
    return engine;
}

void CornellBox::LoadVertexBufferShapesDivided() {
    for (int i = 0; i < N_shapes; i++) {
        std::string name = "shape " + std::to_string(i);
        std::unique_ptr<Shape> shape;

        glm::vec3 color = random_gen.RandomVec3(glm::vec3(0.5f), glm::vec3(1.0f));

        ShapeType shape_type_instance = shape_type;
        if (shape_type_instance == SHAPE_RANDOM)
            shape_type_instance = static_cast<ShapeType>(random_gen.RandomInt(0, 3));
        switch(shape_type_instance) {
            case SHAPE_SPHERE: { // sphere
                float r = random_gen.RandomFloat(0.1f, 0.2f);
                shape = std::make_unique<Sphere>(r, 32, color);
                break;
            }
            case SHAPE_CYLINDER: { // cylinder
                float r = random_gen.RandomFloat(0.1f, 0.2f);
                float h = random_gen.RandomFloat(0.2f, 0.4f);
                shape = std::make_unique<Cylinder>(r, h, 32, color);
                break;
            }
            case SHAPE_BOX: { // box
                glm::vec3 size = random_gen.RandomVec3(glm::vec3(0.2f), glm::vec3(0.4f));
                shape = std::make_unique<Box>(size, color);
                break;
            }
            case SHAPE_CONE: { // cone
                float r = random_gen.RandomFloat(0.1f, 0.2f);
                float h = random_gen.RandomFloat(0.2f, 0.4f);
                shape = std::make_unique<Cone>(r, h, 64, color);
                break;
            }
            default:
                assert(false);
        }

        vertex_datas[name] = shape->GetVertexData();
        vertex_buffers[name] = VertexBuffer(vertex_datas[name].size());
        auto& vd = vertex_datas[name];
        auto& vb = vertex_buffers[name];
        for (int j = 0; j < vd.size(); j++) {
            vb[j] = &vd[j];
        }

        shapes.emplace_back(std::move(shape));
    }
}

void CornellBox::LoadVertexBufferShapesCombined() {
    LoadVertexBufferShapesDivided();

    for (auto& nvd: vertex_datas) {
        auto& vertices = nvd.second;
        glm::vec3 translation = random_gen.RandomVec3(t_min, t_max);
        for (auto& vertex: vertices) vertex.position += translation;
    }

    VertexBuffer vertex_buffers_combined;
    for (auto& nvb: vertex_buffers) {
        auto& vertex_buffer = nvb.second;
        vertex_buffers_combined.insert(
            vertex_buffers_combined.end(), vertex_buffer.begin(), vertex_buffer.end());
    }

    vertex_buffers.clear();
    vertex_buffers["combined"] = vertex_buffers_combined;
}
