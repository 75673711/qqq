#include "linerender.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QDateTime>
#include <QDebug>

#include <windows.h>
#include <stdio.h>

float linepoints[] = { 0.0f, -1.0f,
                       0.0f, 1.0f };

const int max_line_count = 100;

static const char *lineVerTextShader =
        "#version 330\n"
        "layout (location = 0) in vec2 linepoints;\n"
        "layout (location = 1) in vec3 linedata;\n"
        "out float b_color;\n"
        "void main() {\n"
        "   b_color = linedata.z;"
        "   float tempy = (linepoints.y > 0) ? linedata.y - 1.0 : linepoints.y;\n "
        "   gl_Position = vec4(linepoints.x + linedata.x, tempy, 0.0f, 1.0f);\n"
        "}\n";

static const char *lineFragmentShader =
        "#version 330\n"
        "out vec4 frag_color;\n"
        "in float b_color;\n"
        "void main() {\n"
        "   frag_color = vec4(0.0, 1.0, 0.0, b_color);\n"
        "}\n";

LineRender::LineRender() :
    is_inited_(false),
    offset_x_loc_(0),
    offset_x_(0)
{

}

void LineRender::EnsureInit()
{
    if (!is_inited_)
        InitBuf();
}

void LineRender::InitBuf()
{
    is_inited_ = true;

    QOpenGLFunctions_3_3_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();


    // VAO VBO
    f->glGenVertexArrays(1, &VAO);
    f->glBindVertexArray(VAO);

    f->glGenBuffers(1, &VBO);
    f->glBindBuffer(GL_ARRAY_BUFFER, VBO);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(linepoints), linepoints, GL_STATIC_DRAW);

    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    f->glEnableVertexAttribArray(0);

    f->glBindBuffer(GL_ARRAY_BUFFER, 0);

    // line data
//    float line_data_buffer[] = { -0.5f, 1.0f, 1.0f,  //x length opacity
//                           0.5f, 0.5f, 1.0f };

    f->glGenBuffers(1, &line_data_VBO);
    f->glBindBuffer(GL_ARRAY_BUFFER, line_data_VBO);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(float) * max_line_count, NULL, GL_DYNAMIC_DRAW);

    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

    f->glBindBuffer(GL_ARRAY_BUFFER, 0);
    f->glVertexAttribDivisor(1, 1);

    // shader
    shader_program_.reset(new QOpenGLShaderProgram);
    shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex, lineVerTextShader);
    shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment, lineFragmentShader);
    shader_program_->bindAttributeLocation("linepoints", 0);
    shader_program_->link();

    f->glBindVertexArray(0);
}

void LineRender::DrawLines(qint64 utc)
{
    PrintFps();

    EnsureInit();

    QOpenGLFunctions_3_3_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();

    shader_program_->bind();
    f->glBindVertexArray(VAO);

    int total_line = 30;
    int one_unit = 5;
    int unit_count = total_line / one_unit;

    float m_len = 2.0 / total_line;
    float u_len = 2.0 / unit_count;


    int total_sec = 6;  //30
    int unit_sec = total_sec / unit_count;

    SYSTEMTIME st, lt;

    //GetSystemTime(&st);
    GetLocalTime(&lt);

    int offset_msec = (lt.wSecond % unit_sec) * 1000 + lt.wMilliseconds;
    //float offset_x = offset_msec * u_len / (1000 * unit_sec);

    static float temp = 0.0;
    if (temp >= 2.0)
    {
        temp = -1.0;
    }
    else
    {
        temp += 0.01;
    }
    float offset_x = temp;

    static float* line_data_buffer = new float[3 * total_line];
    for (int i = 0; i < total_line / one_unit; ++i)
    {
        for (int j = 0; j < one_unit; ++j)
        {
            int cur = ((i * one_unit) + j) * 3;
            line_data_buffer[cur]     = u_len * i + j * m_len - 1.0 + offset_x;  // x
            line_data_buffer[cur + 1] = (j == 0 ? 1.0 : 0.5); // length
            line_data_buffer[cur + 2] = 1.0; // opacity
        }
    }

//    qDebug() << "---------";
//    for (int i = 0; i < 3 * total_line; i += 3)
//    {
//        qDebug() << line_data_buffer[i] << line_data_buffer[i+1] << line_data_buffer[i+2];
//    }

//    float line_data_buffer[] = { -0.5f, temp, 1.0f,  //x length opacity
//                                 0.5f, 0.5f, temp,
//                                 0.3f, 0.5f, temp};

    f->glBindBuffer(GL_ARRAY_BUFFER, line_data_VBO);
    f->glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * total_line, line_data_buffer);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);


    f->glDrawArraysInstanced(GL_LINES, 0, 2, total_line);
    //f->glDrawArrays(GL_LINES, 0, 2);

    f->glBindVertexArray(0);
    shader_program_->release();
}

void LineRender::DrawLines_1(qint64 utc)
{    
    PrintFps();

    EnsureInit();

    QOpenGLFunctions_3_3_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();

    shader_program_->bind();
    f->glBindVertexArray(VAO);

    int total_line = 30;
    int one_unit = 5;
    int unit_count = total_line / one_unit;

    float m_len = 2.0 / total_line;
    float u_len = 2.0 / unit_count;


    int total_sec = 6;  //30
    int unit_sec = total_sec / unit_count;

    utc = QDateTime::currentMSecsSinceEpoch();
    int offset_msec = utc % (1000 * unit_sec);
    float offset_x = offset_msec * u_len / (1000 * unit_sec);

    static float* line_data_buffer = new float[3 * total_line];
    for (int i = 0; i < total_line / one_unit / 2; ++i)
    {
        for (int j = 0; j < one_unit; ++j)
        {
            int cur = ((i * one_unit) + j) * 3;
            line_data_buffer[cur]     = u_len * i + j * m_len - 1.0 + offset_x;  // x
            line_data_buffer[cur + 1] = (j == 0 ? 1.0 : 0.5); // length
            line_data_buffer[cur + 2] = 1.0; // opacity
        }
    }

    for (int i = 0; i < total_line / one_unit / 2; ++i)
    {
        for (int j = 0; j < one_unit; ++j)
        {
            int cur = ((i * one_unit) + j + total_line / 2) * 3;
            line_data_buffer[cur]     = u_len * i + j * m_len + offset_x;  // x
            line_data_buffer[cur + 1] = (j == 0 ? 1.0 : 0.5); // length
            line_data_buffer[cur + 2] = 1.0; // opacity
        }
    }

//    qDebug() << "---------";
//    for (int i = 0; i < 3 * total_line; i += 3)
//    {
//        qDebug() << line_data_buffer[i] << line_data_buffer[i+1] << line_data_buffer[i+2];
//    }

//    float line_data_buffer[] = { -0.5f, temp, 1.0f,  //x length opacity
//                                 0.5f, 0.5f, temp,
//                                 0.3f, 0.5f, temp};

    f->glBindBuffer(GL_ARRAY_BUFFER, line_data_VBO);
    f->glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * total_line, line_data_buffer);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);


    f->glDrawArraysInstanced(GL_LINES, 0, 2, total_line);
    //f->glDrawArrays(GL_LINES, 0, 2);

    f->glBindVertexArray(0);
    shader_program_->release();
}

void LineRender::PrintFps()
{
    static int draw_c = 0;

    static int sec = 0;
    int now_sec = QDateTime::currentDateTime().time().second();

    if (sec != now_sec)
    {
        qDebug() << draw_c;
        sec = now_sec;
        draw_c = 0;
    }
    else
    {
        ++draw_c;
    }
}

//offset_x_loc_ = shader_program_->uniformLocation("offset_x_");

//shader_program_->setUniformValue(offset_x_loc_, (GLfloat)offset_x_);
