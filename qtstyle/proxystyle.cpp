class MyStyle : public QProxyStyle {
   public:
    void drawPrimitive(QStyle::PrimitiveElement element,
                       const QStyleOption* option,
                       QPainter* painter,
                       const QWidget* widget = nullptr) const {
      if (element == QStyle::PE_FrameFocusRect)
        return;
      return QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
  };
