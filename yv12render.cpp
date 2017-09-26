#include "yv12render.h"

#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QBitmap>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLFunctions_3_3_Core>

#include <QDebug>
#include "previewthread.h"

static unsigned int indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};

static GLfloat vertices[] = {
    //    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // 左上
    //    -1.0f, -1.0f, 0.0f, 0.0, 0.0f,  // 左下
    //    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,    // 右上
    //    1.0f, -1.0f, 0.0f, 1.0f, 0.0f  // 右下
    // 纹理坐标上下颠倒
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // 右上
    1.0f, -1.0f, 0.0f, 1.0, 0.0f,  // 右下
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,    // 左下
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // 左上
};

static const char *VertexShader =
        "#version 330\n"
        "layout (location = 0) in vec3 vertices;\n"
        "layout (location = 1) in vec2 texCoord;\n"
        "out vec2 outTexCoord;\n"
        "void main() {\n"
        "   gl_Position = vec4(vertices, 1.0);\n"
        "   outTexCoord = vec2(texCoord.x, texCoord.y);\n"
        "}\n";


static const char *FragmentShader =
        "#version 330\n"
        "out vec4 FragColor;\n"
        "in vec2 outTexCoord;\n"
        "uniform sampler2D testTexture1;\n"
        "uniform sampler2D testTexture2;\n"
        "uniform sampler2D testTexture3;\n"
        "void main() {\n"
        //"   FragColor = texture(testTexture1, outTexCoord);\n"
        //"gl_FragColor = mix(texture(testTexture1, outTexCoord), texture(testTexture3, outTexCoord), 0.5);\n"

                "  vec4 c = vec4((texture2D(testTexture1, outTexCoord).r - 16./255.) * 1.164);\n"
                "  vec4 U = vec4(texture2D(testTexture2, outTexCoord).r - 128./255.);\n"
                "  vec4 V = vec4(texture2D(testTexture3, outTexCoord).r - 128./255.);\n"
                "  c += V * vec4(1.596, -0.813, 0, 0);\n"
                "  c += U * vec4(0, -0.392, 2.017, 0);\n"
                "  c.a = 1.0;\n"
                "  FragColor = c;\n"

        //"   gl_FragColor = texture(testTexture1, outTexCoord);\n"
        //"gl_FragColor = vec4(1.0, 1.0, 1.0, texture(testTexture, outTexCoord).r);\n"

        "}\n";

YV12Render::YV12Render():
    is_inited_(false)
{

}

void YV12Render::EnsureInit(PaintStruct* ptr_paint_struct)
{
    if (!is_inited_)
    {
        is_inited_ = true;

        QOpenGLFunctions_3_3_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();

        // shader
        int vertexShader = f->glCreateShader(GL_VERTEX_SHADER);
        f->glShaderSource(vertexShader, 1, &VertexShader, NULL);
        f->glCompileShader(vertexShader);

        // fragment shader
        int fragmentShader = f->glCreateShader(GL_FRAGMENT_SHADER);
        f->glShaderSource(fragmentShader, 1, &FragmentShader, NULL);
        f->glCompileShader(fragmentShader);

        shaderProgram = f->glCreateProgram();
        f->glAttachShader(shaderProgram, vertexShader);
        f->glAttachShader(shaderProgram, fragmentShader);
        f->glLinkProgram(shaderProgram);

        f->glDeleteShader(vertexShader);
        f->glDeleteShader(fragmentShader);

        f->glUseProgram(shaderProgram);

        // VAO VBO EBO
        f->glGenVertexArrays(1, &VAO);
        f->glGenBuffers(1, &VBO);
        f->glGenBuffers(1, &EBO);

        f->glBindVertexArray(VAO);

        f->glBindBuffer(GL_ARRAY_BUFFER, VBO);
        f->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        f->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // position attribute
        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        f->glEnableVertexAttribArray(0);
        // texture coord attribute
        f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        f->glEnableVertexAttribArray(1);


        int width = ptr_paint_struct->width;
        int height = ptr_paint_struct->height;
        {
            // texture 1
            // ---------
            f->glGenTextures(1, &texture1_id_);
            f->glBindTexture(GL_TEXTURE_2D, texture1_id_);
            // set the texture wrapping parameters
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer);
            f->glGenerateMipmap(GL_TEXTURE_2D);

            f->glUniform1i(f->glGetUniformLocation(shaderProgram, "testTexture1"), 0);
        }

        {
            f->glGenTextures(1, &texture2_id_);
            f->glBindTexture(GL_TEXTURE_2D, texture2_id_);
            // set the texture wrapping parameters
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer + width * height);
            f->glGenerateMipmap(GL_TEXTURE_2D);

            f->glUniform1i(f->glGetUniformLocation(shaderProgram, "testTexture2"), 1);
        }

        {
            f->glGenTextures(1, &texture3_id_);
            f->glBindTexture(GL_TEXTURE_2D, texture3_id_);
            // set the texture wrapping parameters
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer + width * height * 5 / 4);
            f->glGenerateMipmap(GL_TEXTURE_2D);

            f->glUniform1i(f->glGetUniformLocation(shaderProgram, "testTexture3"), 2);
        }


        f->glUseProgram(0);
        f->glBindVertexArray(0);
    }
}

void YV12Render::Render(PaintStruct* ptr_paint_struct, QOpenGLFunctions* ft)
{
    EnsureInit(ptr_paint_struct);

    QOpenGLFunctions_3_3_Core* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();

    f->glUseProgram(shaderProgram);
    f->glBindVertexArray(VAO);

    int width = ptr_paint_struct->width;
    int height = ptr_paint_struct->height;

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, texture1_id_);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer);
    //f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer);

    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, texture2_id_);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,width / 2, height / 2, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer + width * height);
    //f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer + width * height);


    f->glActiveTexture(GL_TEXTURE2);
    f->glBindTexture(GL_TEXTURE_2D, texture3_id_);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,width / 2, height / 2, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer + width * height * 5 / 4);
    //f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (unsigned char*)ptr_paint_struct->data_buffer + width * height * 5 / 4);

    f->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // 按顺序
    f->glActiveTexture(GL_TEXTURE2);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    f->glBindVertexArray(0);
    f->glUseProgram(0);
}

//ptr_texture1_->destroy();
//ptr_texture1_->create();
//QImage temp("./2.jpg");
//QImage image = temp.convertToFormat(QImage::Format_RGBA8888);

//QOpenGLContext *context = QOpenGLContext::currentContext();
//if (context->isOpenGLES() && context->format().majorVersion() < 3)
//{
//    ptr_texture1_->setFormat(QOpenGLTexture::RGBAFormat);
//}
//else
//{
//    ptr_texture1_->setFormat(QOpenGLTexture::RGBA8_UNorm);
//}

//ptr_texture1_->setSize(temp.width(), temp.height());
//ptr_texture1_->setMipLevels(ptr_texture1_->maximumMipLevels());
//ptr_texture1_->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);

//ptr_texture1_->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, image.constBits());
//ptr_texture1_->bind();

//GLuint texYId;
//GLuint texUId;
//GLuint texVId;

//int width = ptr_paint_struct->width;
//int height = ptr_paint_struct->height;
//unsigned char* buffer = (unsigned char*)ptr_paint_struct->data_buffer;

//glGenTextures ( 1, &texYId );
//glBindTexture ( GL_TEXTURE_2D, texYId );
//glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

//glGenTextures ( 1, &texUId );
//glBindTexture ( GL_TEXTURE_2D, texUId );
//glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer + width * height);
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

//glGenTextures ( 1, &texVId );
//glBindTexture ( GL_TEXTURE_2D, texVId );
//glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer + width * height * 5 / 4 );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );


//        "precision mediump float;\n"
//        "uniform sampler2D tex_y;                 \n"
//        "uniform sampler2D tex_u;                 \n"
//        "uniform sampler2D tex_v;                 \n"
//        "varying vec2 tc;                         \n"
//        "void main()                                  \n"
//        "{                                            \n"
//        "  vec4 c = vec4((texture2D(tex_y, tc).r - 16./255.) * 1.164);\n"
//        "  vec4 U = vec4(texture2D(tex_u, tc).r - 128./255.);\n"
//        "  vec4 V = vec4(texture2D(tex_v, tc).r - 128./255.);\n"
//        "  c += V * vec4(1.596, -0.813, 0, 0);\n"
//        "  c += U * vec4(0, -0.392, 2.017, 0);\n"
//        "  c.a = 1.0;\n"
//        "  gl_FragColor = c;\n"
//        "}                                            \n";

//        "attribute vec3 vertices;    \n"
//        "attribute vec2 a_texCoord;   \n"
//        "varying vec2 tc;     \n"
//        "void main()                  \n"
//        "{                            \n"
//        "   gl_Position = vec4(rectpoints, 1.0f);  \n"
//        "   tc = a_texCoord;  \n"
//        "}                            \n";

// 1
//{
//    ptr_texture1_->destroy();
//    ptr_texture1_->create();
////        ptr_texture1_->setFormat(QOpenGLTexture::RGBA8_UNorm);

////        QImage temp("./1.jpg");
////        QImage image = temp.convertToFormat(QImage::Format_RGBA8888);


////        ptr_texture1_->setSize(image.width(), image.height());
////        ptr_texture1_->setMipLevels(ptr_texture1_->maximumMipLevels());
////        ptr_texture1_->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);

////        ptr_texture1_->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, image.constBits());
////        ptr_texture1_->bind(0);

//    ptr_texture1_->setData(QImage("./1.jpg"));
//    ptr_texture1_->bind(0);
//    shader_program_->setUniformValue("testTexture1", 0);
//}

//// 2
//{
//    ptr_texture2_->destroy();
//    ptr_texture2_->create();
////        ptr_texture2_->setFormat(QOpenGLTexture::RGBA8_UNorm);

////        QImage temp("./2.jpg");
////        QImage image = temp.convertToFormat(QImage::Format_RGBA8888);


////        ptr_texture2_->setSize(image.width(), image.height());
////        ptr_texture2_->setMipLevels(ptr_texture2_->maximumMipLevels());
////        ptr_texture2_->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);

////        ptr_texture2_->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, image.constBits());
////        ptr_texture2_->bind(1);

//    ptr_texture2_->setData(QImage("./2.jpg"));
//    ptr_texture2_->bind(1);
//    shader_program_->setUniformValue("testTexture2", 1);
//}
