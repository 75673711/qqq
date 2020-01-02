#include "linerender.h"

#include <QOpenGLFunctions_4_4_Compatibility>
#include <QDateTime>
#include <QDebug>
#include <QColor>
#include <QPair>
#include "textrender/fasttextrender.h"

#include <windows.h>
#include <stdio.h>


float linepoints[] = { 0.0f, -1.0f,
                       0.0f, 1.0f };

const int max_line_count = 200;

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
    offset_x_(0),
    line_data_buffer(NULL)
{
    ptr_text_render_ = new FastTextRender;
    ptr_text_render_->SetFontSize(20);
}

void LineRender::EnsureInit()
{
    if (!is_inited_)
        InitBuf();
}

void LineRender::UninitBuf()
{
    if (is_inited_)
    {
        delete [] line_data_buffer;
        line_data_buffer = NULL;

        //  vao vbo shader texture
    }
}

void LineRender::InitBuf()
{
    is_inited_ = true;

    line_data_buffer = new float[max_line_count * 3];

    QOpenGLFunctions_4_4_Compatibility* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_4_Compatibility>();

    // VAO VBO
    f->glGenVertexArrays(1, &VAO);
    f->glBindVertexArray(VAO);

    // line point
    f->glGenBuffers(1, &VBO);
    f->glBindBuffer(GL_ARRAY_BUFFER, VBO);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(linepoints), linepoints, GL_STATIC_DRAW);

    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    f->glEnableVertexAttribArray(0);

    f->glBindBuffer(GL_ARRAY_BUFFER, 0);

    // line data
    f->glGenBuffers(1, &line_data_VBO);
    f->glBindBuffer(GL_ARRAY_BUFFER, line_data_VBO);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(float) * max_line_count * 3, NULL, GL_STATIC_DRAW);

    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    f->glEnableVertexAttribArray(1);

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

void LineRender::DrawLines(qint64 utc,
               const QSize& view_size,
               int total_width,
               int total_sec,
               int total_line,
               int one_unit_line,
               int line_height)
{
    EnsureInit();

    QOpenGLFunctions_4_4_Compatibility* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_4_Compatibility>();

    shader_program_->bind();
    f->glBindVertexArray(VAO);

    float total_length = total_width * 2.0 / view_size.width();
    float long_line_height = line_height * 2.0 / view_size.height();
    float short_line_height = long_line_height / 2.0;

    int unit_count = total_line / one_unit_line;

    float m_len = total_length / total_line;
    float u_len = total_length / unit_count;

    int unit_msec = total_sec * 1000 / unit_count;
    int min_msec = unit_msec / one_unit_line;

    int offset_msec = utc % unit_msec;
    float offset_x = - offset_msec * total_length / (1000 * total_sec);

    int index = 0;   // visible line index
    QVector<QPair<float, qint64>> time_x_vec;
    // left
    for (int i = total_line / one_unit_line; i >= 0; --i)
    {
        for (int j = 0; j < one_unit_line; ++j)
        {
            int cur = index * 3;

            float x = -u_len * i + j * m_len - (u_len - offset_x);
            if (x < -1.0 || x > 1.0)
            {
                continue;
            }
            else
            {
                line_data_buffer[cur] = x;  // x

                float length = (j == 0 ? long_line_height : short_line_height);
                line_data_buffer[cur + 1] = length; // length

                if (length == long_line_height)
                {
                    // need draw time
                    qint64 c_utc = utc - i * unit_msec + j * min_msec - unit_msec - offset_msec;
                    time_x_vec.append(qMakePair(x, c_utc));
                }

                line_data_buffer[cur + 2] = 1.0; // opacity

                ++index;
            }
        }
    }

    // right
    for (int i = 0; i < total_line / one_unit_line; ++i)
    {
        for (int j = 0; j < one_unit_line; ++j)
        {
            int cur = index * 3;
            float x = u_len * i + j * m_len + offset_x;
            if (x < -1.0 || x > 1.0)
            {
                continue;
            }
            else
            {
                line_data_buffer[cur] = x;  // x

                float length = (j == 0 ? long_line_height : short_line_height);
                line_data_buffer[cur + 1] = length; // length
                if (length == long_line_height)
                {
                    // need draw time
                    qint64 c_utc = utc + i * unit_msec + j * min_msec - offset_msec;
                    time_x_vec.append(qMakePair(x, c_utc));
                }

                line_data_buffer[cur + 2] = 1.0; // opacity

                ++index;
            }
        }
    }

    f->glBindBuffer(GL_ARRAY_BUFFER, line_data_VBO);
    f->glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * index, line_data_buffer);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);

    f->glDrawArraysInstanced(GL_LINES, 0, 2, index);

    f->glBindVertexArray(0);
    shader_program_->release();

    QList<QPair<QString, QPoint>> text_pair_list;
    if (time_x_vec.count() > 0)
    {
        int c_sec = (time_x_vec[0].second / 1000) % 60;

        QPoint pos(0, line_height + 20);
        for (int i = 0; i < time_x_vec.count(); ++i)
        {
            float x = time_x_vec[i].first;
            pos.setX((x + 1.0) * (float)view_size.width() / 2.0);

            QString time = QString::number(c_sec);

            c_sec += unit_msec / 1000;
            if (c_sec >= 60)
            {
                c_sec -= 60;
            }

            text_pair_list << qMakePair(time, pos);
        }
    }
    ptr_text_render_->RenderText(text_pair_list, view_size, QColor(Qt::red));

    QPoint center(view_size.width() / 2, view_size.height() / 2);
    ptr_text_render_->RenderText({qMakePair(QDateTime::fromMSecsSinceEpoch(utc).time().toString("hh:mm:ss"),center )}, view_size, QColor(Qt::green));
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
