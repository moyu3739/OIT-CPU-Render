#include <vector>
#include <glm/glm.hpp>
#define PI 3.14159265

struct ShapeVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

class Shape {
public:
    Shape() = default;

    virtual ~Shape() = default;

    // Get the vertex buffer of the shape
    virtual const std::vector<ShapeVertex>& GetVertexData() const {
        return vertex_data;
    }

    // Get the number of vertices in the shape
    virtual size_t GetVertexCount() const {
        return vertex_data.size();
    }

protected:
    std::vector<ShapeVertex> vertex_data;
};


class Sphere: public Shape {
public:
    Sphere(float radius, int segment, const glm::vec3& color) {
        Update(radius, segment, color);
    }

    void Update(float radius, int segment, const glm::vec3& color) {
        this->radius = radius;
        this->segment = segment;
        this->color = color;

        auto vertices = GetVertices(radius, segment);
        auto indice = GetIndices(segment);
        vertex_data.resize(indice.size());
        for (int i = 0; i < indice.size(); i++) {
            vertex_data[i].position = vertices[indice[i]].position;
            vertex_data[i].normal = vertices[indice[i]].position / radius; // normalize
            vertex_data[i].color = color; // default color
        }
    }

private:
    std::vector<ShapeVertex> GetVertices(float radius_, int segment_) {
        std::vector<ShapeVertex> vertices; 
        int layer = segment_ / 2;
        float deltaLat = PI / layer;
        float deltaLon = 2 * PI / segment_;
        vertices.resize((layer - 1) * segment_ + 2);
        vertices[0].position = glm::vec3(0.0f, radius_, 0.0f);
        vertices[vertices.size() - 1].position = glm::vec3(0.0f, -radius_, 0.0f);
        int vid = 1;
        for (int i = 1; i < layer; i++) {
            float latitude = i * deltaLat;
            for (int j = 0; j < segment_; j++) {
                float longitude = j * deltaLon;
                vertices[vid].position.x = radius_ * sin(latitude) * cos(longitude);
                vertices[vid].position.y = radius_ * cos(latitude);
                vertices[vid].position.z = radius_ * sin(latitude) * sin(longitude);
                vid++;
            }
        }
        return vertices;
        return vertices;
    }

    std::vector<uint32_t> GetIndices(int segment_) {
        std::vector<uint32_t> indices;
        int layer = segment_ / 2;
        int vertex = (layer - 1) * segment_ + 2;
        int plane = 2 * (layer - 1) * segment_;
        indices.resize(3 * plane);

        int fid = 0;
        for (int i = 1; i < segment_; i++) {
            indices[3 * fid] = 0;
            indices[3 * fid + 1] = i + 1;
            indices[3 * fid + 2] = i;
            fid++;
        }
        indices[3 * fid] = 0;
        indices[3 * fid + 1] = 1;
        indices[3 * fid + 2] = segment_;
        fid++;

        for (int i = 1; i < vertex - segment_ - 1; i+=segment_) {
            for (int j = i; j < i + segment_ - 1; j++) {
                indices[3 * fid] = j;
                indices[3 * fid + 1] = j + 1;
                indices[3 * fid + 2] = j + segment_ + 1;
                fid++;
                indices[3 * fid] = j;
                indices[3 * fid + 1] = j + segment_ + 1;
                indices[3 * fid + 2] = j + segment_;
                fid++;
            }
            indices[3 * fid] = i;
            indices[3 * fid + 1] = i + segment_;
            indices[3 * fid + 2] = i + segment_ - 1;
            fid++;
            indices[3 * fid] = i + segment_ - 1;
            indices[3 * fid + 1] = i + segment_;
            indices[3 * fid + 2] = i + 2 * segment_ - 1;
            fid++;
        }

        for (int i = vertex - segment_- 1; i < vertex - 2; i++) {
            indices[3 * fid] = i;
            indices[3 * fid + 1] = i + 1;
            indices[3 * fid + 2] = vertex - 1;
            fid++;
        }
        indices[3 * fid] = vertex - 2;
        indices[3 * fid + 1] = vertex - segment_ - 1;
        indices[3 * fid + 2] = vertex - 1;
        fid++;

        return indices;
    }

public:
    float radius;
    int segment;
    glm::vec3 color;
};


class Cone : public Shape {
public:
    Cone(float radius, float height, int segment, const glm::vec3& color) {
        Update(radius, height, segment, color);
    }

    void Update(float radius, float height, int segment, const glm::vec3& color) {
        this->radius = radius;
        this->height = height;
        this->segment = segment;
        this->color = color;

        float deltaPhi = 2.0f * PI / segment;
        vertex_data.clear();
        for (int i = 0; i < segment - 1; i++) {
            float phi = i * 2.0f * PI / segment;
            ShapeVertex v1, v2, v3;
            v1.position = glm::vec3(0.0f, height, 0.0f);
            v2.position = glm::vec3(radius * cos(phi + deltaPhi), 0.0f, radius * sin(phi + deltaPhi));
            v3.position = glm::vec3(radius * cos(phi), 0.0f, radius * sin(phi));
            v2.normal = OneNormal(v2.position);
            v3.normal = OneNormal(v3.position);
            v1.normal = 0.5f * (v2.normal + v3.normal);
            vertex_data.emplace_back(v1);
            vertex_data.emplace_back(v2);
            vertex_data.emplace_back(v3);
        }
        ShapeVertex v1, v2, v3;
        v1.position = glm::vec3(0.0f, height, 0.0f);
        v2.position = glm::vec3(radius, 0.0f, 0.0f);
        v3.position = glm::vec3(radius * cos(-deltaPhi), 0.0f, radius * sin(-deltaPhi));
        v2.normal = OneNormal(v2.position);
        v3.normal = OneNormal(v3.position);
        v1.normal = 0.5f * (v2.normal + v3.normal);
        vertex_data.emplace_back(v1);
        vertex_data.emplace_back(v2);
        vertex_data.emplace_back(v3);
    
        for (int i = 0; i < segment - 1; i++) {
            float phi = i * 2.0f * PI / segment;
            ShapeVertex v1, v2, v3;
            v1.position = glm::vec3(0.0f, 0.0f, 0.0f);
            v2.position = glm::vec3(radius * cos(phi), 0.0f, radius * sin(phi));
            v3.position = glm::vec3(radius * cos(phi + deltaPhi), 0.0f, radius * sin(phi + deltaPhi));
            v2.normal = glm::vec3(0.0f, -1.0f, 0.0f);
            v3.normal = glm::vec3(0.0f, -1.0f, 0.0f);
            v1.normal = glm::vec3(0.0f, -1.0f, 0.0f);
            vertex_data.emplace_back(v1);
            vertex_data.emplace_back(v2);
            vertex_data.emplace_back(v3);
        }
        v1.position = glm::vec3(0.0f, 0.0f, 0.0f);
        v2.position = glm::vec3(radius * cos(-deltaPhi), 0.0f, radius * sin(-deltaPhi));
        v3.position = glm::vec3(radius, 0.0f, 0.0f);
        v2.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        v3.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        v1.normal = glm::vec3(0.0f, -1.0f, 0.0f);
        vertex_data.emplace_back(v1);
        vertex_data.emplace_back(v2);
        vertex_data.emplace_back(v3);

        for (auto& v : vertex_data) v.color = color;
    }

private:
    glm::vec3 OneNormal(const glm::vec3& v) {
        glm::vec3 skew = glm::vec3(0.0f, height, 0.0f) - v;
        return glm::normalize(glm::cross(glm::cross(skew, v), skew));
    }

public:
    float radius;
    float height;
    int segment;
    glm::vec3 color;
};


class Cylinder : public Shape {
public:
    Cylinder(float radius, float height, int segment, const glm::vec3& color) {
        Update(radius, height, segment, color);
    }

    void Update(float radius, float height, int segment, const glm::vec3& color) {
        this->radius = radius;
        this->height = height;
        this->segment = segment;
        this->color = color;

        auto vertices = GetVertices(radius, height, segment);
        auto indices = GetIndices(segment);
        vertex_data.clear();
        for (int i = 0; i < 2 * segment; i++) {
            ShapeVertex v1, v2, v3;
            v1 = vertices[indices[3 * i]];
            v2 = vertices[indices[3 * i + 1]];
            v3 = vertices[indices[3 * i + 2]];
            v1.normal = i % 2 == 0 ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            v2.normal = v1.normal;
            v3.normal = v1.normal;
            vertex_data.emplace_back(v1);
            vertex_data.emplace_back(v2);
            vertex_data.emplace_back(v3);
        }

        for (int i = 2 * segment; i < 4 * segment; i++) {
            ShapeVertex v1, v2, v3;
            v1 = vertices[indices[3 * i]];
            v2 = vertices[indices[3 * i + 1]];
            v3 = vertices[indices[3 * i + 2]];
            v1.normal = OneNormal(v1.position);
            v2.normal = OneNormal(v2.position);
            v3.normal = OneNormal(v3.position);
            vertex_data.emplace_back(v1);
            vertex_data.emplace_back(v2);
            vertex_data.emplace_back(v3);
        }

        for (auto& v : vertex_data) v.color = color;
    }

private:
    inline glm::vec3 OneNormal(glm::vec3 v) {
        return glm::normalize(glm::vec3(v.x, 0.0f, v.z));
    }

    std::vector<ShapeVertex> GetVertices(float radius_, float height_, int segment_) {
        std::vector<ShapeVertex> vertices;
        vertices.resize(2 * segment_ + 2);
        vertices[0].position = glm::vec3(0.0f, 0.0f, 0.0f);
        vertices[2 * segment_ + 1].position = glm::vec3(0.0f, height_, 0.0f);
        for (int i = 1; i <= segment_; i++) {
            float phi = (i - 1) * 2.0f * PI / segment_;
            vertices[2 * i - 1].position = glm::vec3(radius_ * cos(phi), 0.0f, radius_ * sin(phi));
            vertices[2 * i].position = glm::vec3(radius_ * cos(phi), height_, radius_ * sin(phi));
        }
    
        return vertices;
    }
    
    std::vector<uint32_t> GetIndices(int segment_) {
        std::vector<uint32_t> indices;
        indices.clear();
    
        for (int i = 1; i < segment_; i++) {
            indices.emplace_back(0);
            indices.emplace_back(2 * i - 1);
            indices.emplace_back(2 * i + 1);
            indices.emplace_back(2 * segment_ + 1);
            indices.emplace_back(2 * i + 2);
            indices.emplace_back(2 * i);
        }
        indices.emplace_back(0);
        indices.emplace_back(2 * segment_ - 1);
        indices.emplace_back(1);
        indices.emplace_back(2 * segment_ + 1);
        indices.emplace_back(2);
        indices.emplace_back(2 * segment_);
    
        for (int i = 1; i < segment_; i++) {
            uint32_t idx_b1, idx_b2, idx_t1, idx_t2;
            idx_b1 = 2 * i - 1;
            idx_b2 = 2 * i + 1;
            idx_t1 = 2 * i;
            idx_t2 = 2 * i + 2;
            indices.emplace_back(idx_b1);
            indices.emplace_back(idx_t1);
            indices.emplace_back(idx_t2);
            indices.emplace_back(idx_b1);
            indices.emplace_back(idx_t2);
            indices.emplace_back(idx_b2);
        }
        uint32_t idx_b1, idx_b2, idx_t1, idx_t2;
        idx_b1 = 2 * segment_ - 1;
        idx_b2 = 1;
        idx_t1 = 2 * segment_;
        idx_t2 = 2;
        indices.emplace_back(idx_b1);
        indices.emplace_back(idx_t1);
        indices.emplace_back(idx_t2);
        indices.emplace_back(idx_b1);
        indices.emplace_back(idx_t2);
        indices.emplace_back(idx_b2);
    
        return indices;
    }

public:
    float radius;
    // float top_radius;
    // float bottom_radius;
    float height;
    int segment;
    glm::vec3 color;
};


class Box : public Shape {
public:
    Box(const glm::vec3& size, const glm::vec3& color) {
        Update(size.x, size.y, size.z, color);
    }

    Box(float lx, float ly, float lz, const glm::vec3& color) {
        Update(lx, ly, lz, color);
    }

    void Update(float lx, float ly, float lz, const glm::vec3& color) {
        this->lx = lx;
        this->ly = ly;
        this->lz = lz;
        this->color = color;

        auto vertices = GetVertices(lx, ly, lz);
        auto indices = GetIndices();
        auto normals = GetNormals();
        vertex_data.resize(indices.size());
        for (int i = 0; i < indices.size(); i++) {
            vertex_data[i].position = vertices[indices[i]].position;
            vertex_data[i].normal =  normals[i / 6]; // 6 vertices share the same normal
            vertex_data[i].color = color; // default color
        }
    }

    std::vector<ShapeVertex> GetVertices(float lx_, float ly_, float lz_) {
        std::vector<ShapeVertex> vertices;
        vertices.resize(8);
        vertices[0].position = glm::vec3(-lx_ / 2, 0, lz_ / 2);
        vertices[1].position = glm::vec3(lx_ / 2, 0, lz_ / 2);
        vertices[2].position = glm::vec3(lx_ / 2, 0, -lz_ / 2);
        vertices[3].position = glm::vec3(-lx_ / 2, 0, -lz_ / 2);
        vertices[4].position = glm::vec3(-lx_ / 2, ly_, lz_ / 2);
        vertices[5].position = glm::vec3(lx_ / 2, ly_, lz_ / 2);
        vertices[6].position = glm::vec3(lx_ / 2, ly_, -lz_ / 2);
        vertices[7].position = glm::vec3(-lx_ / 2, ly_, -lz_ / 2);
        return vertices;
    }

    std::vector<uint32_t> GetIndices() {
        std::vector<uint32_t> indices{
            0,2,1,
            0,3,2,
            0,1,4,
            1,5,4,
            1,2,5,
            2,6,5,
            0,4,3,
            3,4,7,
            2,3,6,
            3,7,6,
            4,5,6,
            4,6,7
        };
        return indices;
    }

    std::vector<glm::vec3> GetNormals() {
        std::vector<glm::vec3> normals;
        normals.resize(6);
        normals[0] = glm::vec3(0.0f, -1.0f, 0.0f);
        normals[1] = glm::vec3(0.0f, 0.0f, 1.0f);
        normals[2] = glm::vec3(1.0f, 0.0f, 0.0f);
        normals[3] = glm::vec3(-1.0f, 0.0f, 0.0f);
        normals[4] = glm::vec3(0.0f, 0.0f, -1.0f);
        normals[5] = glm::vec3(0.0f, 1.0f, 0.0f);
        return normals;
    }

public:
    float lx;
    float ly;
    float lz;
    glm::vec3 color;
};

