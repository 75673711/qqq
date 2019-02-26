directx backend

Qt Platform Abstraction

性能优化：
1.裁剪clip（使用eliding裁剪文字，QQuickImageProvider 裁剪图标），opacity透明度，尽量不用
2.不可见设置visible=false，透明度图形不重叠
3.创建纹理时使用纹理坐标QQuickWindow::TextureCanUseAtlas
4.尽量使用不透明图标QImage::Format_RGB32
6.使用item代替rectangle,window背景设置color而非大rectangle

环境参数
使用QSG_RENDER_TIMING=1查看各项性能指标

1.信号槽
signal signalTest(bool enable);
on<signal>: { enable // 直接使用信号中声明的形参 }
2.其他线程发出的信号，在断开连接后仍有可能执行槽函数

备忘常用
The QML Reference   总纲
QML Object Attributes  属性
learnopengl
