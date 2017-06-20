#include "TerrainRenderer.h"
#include "TextureManager.h"
#include "Utility.h"
#include "Log.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>

namespace fly
{


TerrainRenderer::TerrainRenderer(ShaderProgram& shader, int radius, int detail) :
        m_shaderProgram(shader),
        m_radius(radius),
        m_detail(detail)
{
    glGenVertexArrays(1, &m_vertexArrayObject);
    glBindVertexArray(m_vertexArrayObject);

    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glGenBuffers(1, &m_elementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);

    if (m_shaderProgram.loadShaderFile("shaders/shader.vert", Shader::Vertex))
        LOG(Info) << "Loaded Vertex shader" << std::endl;
    if (m_shaderProgram.loadShaderFile("shaders/shader.frag", Shader::Fragment))
        LOG(Info) << "Loaded Fragment shader" << std::endl;

    ASSERT_GL_ERRORS();
}

TerrainRenderer::~TerrainRenderer()
{
    glDeleteBuffers(1, &m_vertexBuffer);
    glDeleteBuffers(1, &m_elementBuffer);
    glDeleteVertexArrays(1, &m_vertexArrayObject);
    ASSERT_GL_ERRORS();
}

void TerrainRenderer::reset(int radius, int detail)
{
    m_radius = radius;
    m_detail = detail;

    m_chunks = sq(2 * radius - 1);
    m_elementsPerChunk = 2 * 3 * sq(detail);
    m_verticesPerChunk = sq(detail + 1); //4 * sq(detail); FIXME

    LOG(Info) << "Terrain radius: " << m_radius << std::endl;
    LOG(Info) << "Terrain detail: " << m_detail << std::endl;
    LOG(Info) << "Terrain chunks: " << m_chunks << std::endl;
    LOG(Info) << "Elements per chunk: " << m_elementsPerChunk << std::endl;
    LOG(Info) << "Vertices per chunk: " << m_verticesPerChunk << std::endl;

    glBindVertexArray(m_vertexArrayObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_verticesPerChunk * m_chunks,
                 nullptr, GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_elementsPerChunk * m_chunks,
                 nullptr, GL_DYNAMIC_DRAW);

    m_shaderProgram.setAttributeFloat("position", 3, sizeof(Vertex), offsetof(Vertex, position));
    m_shaderProgram.setAttributeFloat("texcoords", 2, sizeof(Vertex), offsetof(Vertex, texcoords));

    m_shaderProgram.use();
    /* Move this setUniforms to constructor ? */
    m_shaderProgram.setUniform("tex", TextureManager::getSampler("resources/texture.png"));
    m_shaderProgram.setUniform("model", glm::mat4(1.f));

    ASSERT_GL_ERRORS();
}

void TerrainRenderer::draw()
{
    glBindVertexArray(m_vertexArrayObject);
    m_shaderProgram.use();
    glDrawElements(GL_TRIANGLES, m_elementsPerChunk * m_chunks, GL_UNSIGNED_INT, 0);
}

void TerrainRenderer::updateChunk(int chunk_x, int chunk_y, int coord_x, int coord_y, const std::vector<float>& chunk_heights)
{
    std::vector<Vertex> vertices;
    vertices.reserve(m_verticesPerChunk);
    for (int i = 0, c = 0; i <= m_detail; ++i)
    {
        for (int j = 0; j <= m_detail; ++j, ++c)
        {
            float x = coord_x + 1.0f * i / (float)m_detail - 0.5f,
                  y = coord_y + 1.0f * j / (float)m_detail - 0.5f;
            GLfloat height = chunk_heights[c];
            vertices.push_back({{x, y, height}, {x - coord_x, y - coord_y}});
        }
    }

    auto offset = (chunk_x + m_radius - 1) * (2 * m_radius - 1) + (chunk_y + m_radius - 1);
//     LOG(Debug) << "Offset: " << offset << std::endl;

    std::vector<GLuint> elements;
    elements.reserve(m_elementsPerChunk);
    for (int i = 0, vertex_offset = m_verticesPerChunk * offset; i < m_detail; ++i)
    {
        for (int j = 0; j < m_detail; ++j)
        {
            elements.push_back(vertex_offset + (m_detail + 1) * i + j);
            elements.push_back(vertex_offset + (m_detail + 1) * i + j + 1);
            elements.push_back(vertex_offset + (m_detail + 1) * (i + 1) + j);

            elements.push_back(vertex_offset + (m_detail + 1) * (i + 1) + j + 1);
            elements.push_back(vertex_offset + (m_detail + 1) * i + j + 1);
            elements.push_back(vertex_offset + (m_detail + 1) * (i + 1) + j);
        }
    }

    assert(elements.size() == m_elementsPerChunk);
    assert(vertices.size() == m_verticesPerChunk);


    glBindVertexArray(m_vertexArrayObject);

    glBufferSubData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_verticesPerChunk * offset,
                    sizeof(Vertex) * m_verticesPerChunk, vertices.data());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * m_elementsPerChunk * offset,
                    sizeof(GLuint) * m_elementsPerChunk, elements.data());

    ASSERT_GL_ERRORS();
}


}
