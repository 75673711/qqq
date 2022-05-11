#include "cef_view.h"

#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QFile>

#include "cef/include/bind_helper.h"
#include "cef/include/cef_app.h"
#include "cef/include/cef_parser.h"
#include "cef/include/wrapper/cef_closure_task.h"
#include "cef/include/wrapper/cef_helpers.h"
#include "utility/os_util.h"

#if defined(OS_LINUX)
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <linux/input.h>
#endif

#include "biz/biz_others/i18n/i18n_manager.h"
#include "biz/open_service/scheme_service.h"

namespace client_ding {

void RunCallback(cef_base::Callback<void(QWidget*)> callback, QWidget* window) { callback.Run(window); }

std::string GetDescriptionFromMimeType(const std::string& mime_type) {
  // Check for wild card mime types and return an appropriate description.
  struct kWildCardMimeType {
    const char* mime_type;
    const char* label;
  };

  static const kWildCardMimeType kWildCardMimeTypes[] = {
      {"audio", "Audio Files"},
      {"image", "Image Files"},
      {"text", "Text Files"},
      {"video", "Video Files"},
  };

  for (size_t i = 0; i < sizeof(kWildCardMimeTypes) / sizeof(kWildCardMimeTypes[0]); ++i) {
    if (mime_type == std::string(kWildCardMimeTypes[i].mime_type) + "/*") {
      return std::string(kWildCardMimeTypes[i].label);
    }
  }

  return std::string();
}

QStringList MakeFilters(const std::vector<CefString>& accept_filters, bool include_all_files) {
  QStringList out_list;
  bool has_filter = false;

  for (size_t i = 0; i < accept_filters.size(); ++i) {
    const std::string& filter = accept_filters[i];
    if (filter.empty()) continue;

    std::vector<std::string> extensions;
    std::string description;

    size_t sep_index = filter.find('|');
    if (sep_index != std::string::npos) {
      // Treat as a filter of the form "Filter Name|.ext1;.ext2;.ext3".
      description = filter.substr(0, sep_index);

      const std::string& exts = filter.substr(sep_index + 1);
      size_t last = 0;
      size_t size = exts.size();
      for (size_t i = 0; i <= size; ++i) {
        if (i == size || exts[i] == ';') {
          std::string ext(exts, last, i - last);
          if (!ext.empty() && ext[0] == '.') extensions.push_back(ext);
          last = i + 1;
        }
      }
    } else if (filter[0] == '.') {
      // Treat as an extension beginning with the '.' character.
      extensions.push_back(filter);
    } else {
      // Otherwise convert mime type to one or more extensions.
      description = GetDescriptionFromMimeType(filter);

      std::vector<CefString> ext;
      CefGetExtensionsForMimeType(filter, ext);
      for (size_t x = 0; x < ext.size(); ++x) extensions.push_back("." + ext[x].ToString());
    }

    if (extensions.empty()) continue;

    std::string ext_str;
    for (size_t x = 0; x < extensions.size(); ++x) {
      const std::string& pattern = "*" + extensions[x];
      if (x != 0) ext_str += " ";
      ext_str += pattern;
    }

    if (description.empty())
      description = ext_str;
    else
      description += " (" + ext_str + ")";

    out_list.append(description.c_str());
    if (!has_filter) has_filter = true;
  }

  // Add the *.* filter, but only if we have added other filters (otherwise it
  // is implied).
  if (include_all_files && has_filter) {
    out_list.append("All Files (*)");
  }

  return out_list;
}

bool NativeDialogHandler::OnFileDialog(CefRefPtr<CefBrowser> browser, FileDialogMode mode, const CefString& title,
                                       const CefString& default_file_path, const std::vector<CefString>& accept_filters,
                                       int selected_accept_filter, CefRefPtr<CefFileDialogCallback> callback) {
  CEF_REQUIRE_UI_THREAD();

  OnNativeFileDialogParams params;
  params.browser = browser;
  params.mode = mode;
  params.title = title;
  params.default_file_path = default_file_path;
  params.accept_filters = accept_filters;
  params.selected_accept_filter = selected_accept_filter;
  params.callback = callback;

  GetWindowAndContinue(browser, cef_base::Bind(&NativeDialogHandler::OnFileDialogContinue, this, params));
  return true;
}

void NativeDialogHandler::OnFileDialogContinue(OnNativeFileDialogParams params, QWidget* window) {
  CEF_REQUIRE_UI_THREAD();

  std::vector<CefString> files;
  FileDialogMode mode_type = static_cast<FileDialogMode>(params.mode & FILE_DIALOG_TYPE_MASK);

  QFileDialog dialog(window);
  if (mode_type == FILE_DIALOG_OPEN || mode_type == FILE_DIALOG_OPEN_MULTIPLE) {
    // getOpenFileName
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    if (mode_type == FILE_DIALOG_OPEN_MULTIPLE) {
      dialog.setFileMode(QFileDialog::ExistingFiles);
    } else {
      dialog.setFileMode(QFileDialog::ExistingFile);
    }
  } else if (mode_type == FILE_DIALOG_OPEN_FOLDER) {
    // getExistingDirectory
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
  } else if (mode_type == FILE_DIALOG_SAVE) {
    // getSaveFileName
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    if (!(params.mode & FILE_DIALOG_OVERWRITEPROMPT_FLAG)) {
      dialog.setOption(QFileDialog::DontConfirmOverwrite);
    }
    if (!params.default_file_path.empty()) {
      const std::string& file_path = params.default_file_path;
      dialog.selectFile(file_path.c_str());
    }
  } else {
    NOTREACHED();
    params.callback->Cancel();
    return;
  }

  QStringList q_filters;
  if (params.accept_filters.size() > 0) {
    q_filters = MakeFilters(params.accept_filters, true);

    dialog.setNameFilters(q_filters);
    if (params.selected_accept_filter < static_cast<int>(params.accept_filters.size())) {
      dialog.selectNameFilter(q_filters.at(params.selected_accept_filter));
    }
  }

  int filter_index = params.selected_accept_filter;
  if (dialog.exec() == QDialog::Accepted) {
    QStringList out_files = dialog.selectedFiles();
    for (int i = 0; i < out_files.count(); ++i) {
      files.push_back(out_files[i].toStdString());
    }
    QString name_filter = dialog.selectedNameFilter();
    if (!name_filter.isEmpty()) {
      for (int i = 0; i < q_filters.count(); ++i) {
        if (q_filters.at(i) == name_filter) {
          filter_index = i;
          break;
        }
      }
    }
    params.callback->Continue(filter_index, files);
  } else {
    params.callback->Cancel();
  }
}

#if defined(OS_MAC)
void NativeDialogHandler::GetWindowAndContinue(CefRefPtr<CefBrowser> browser, base::Callback<void(QWidget*)> callback) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    MAIN_POST_CLOSURE(base::Bind(&NativeDialogHandler::GetWindowAndContinue, this, browser, callback));
    return;
  }

  scoped_refptr<client::CefWidget> cef_widget = client::CefWidget::GetForBrowser(browser->GetIdentifier());
  QWidget* widget = QWidget::find((WId)cef_widget->GetHandle());
  if (cef_widget) {
    CefPostTask(TID_UI, base::Bind(RunCallback, callback, widget));
  }
}
#else   // OS_MAC
void NativeDialogHandler::GetWindowAndContinue(CefRefPtr<CefBrowser> browser,
                                               cef_base::Callback<void(QWidget*)> callback) {
  if (!CURRENTLY_ON_MAIN_THREAD()) {
    MAIN_POST_CLOSURE(cef_base::Bind(&NativeDialogHandler::GetWindowAndContinue, this, browser, callback));
    return;
  }

  cef_base::scoped_refptr<CefWidget> cef_widget = CefWidget::GetForBrowser(browser->GetIdentifier());
  QWidget* widget = QWidget::find((WId)cef_widget->GetHandle());
  if (cef_widget) {
    CefPostTask(TID_UI, cef_base::Bind(RunCallback, callback, widget));
  }
}
#endif  // OS_MAC

void NativeContextMenuHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                   CefRefPtr<CefContextMenuParams> params,
                                                   CefRefPtr<CefMenuModel> model) {}

bool NativeContextMenuHandler::RunContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                              CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model,
                                              CefRefPtr<CefRunContextMenuCallback> callback) {
  std::string select_string = params->GetSelectionText();
  if (select_string.empty()) {
    return true;
  }

  CefPostTask(TID_UI, cef_base::Lambda([=]() {
                QMenu* menu = new QMenu();
                QAction* ptr_action_copy_ = menu->addAction(QLS("pc_conv_chat_edit_menu_copy"));
                ptr_action_copy_->setShortcuts(QKeySequence::Copy);
                ptr_action_copy_->setShortcutContext(Qt::ApplicationShortcut);
                ptr_action_copy_->setEnabled(true);
                QObject::connect(ptr_action_copy_, &QAction::triggered, new QObject(), [=]() {
                  QClipboard* board = QApplication::clipboard();
                  board->setText(QString(select_string.c_str()));
                });

                QFile qss(":/qss/qmenu.qss");
                if (qss.open(QFile::ReadOnly)) {
                  menu->setStyleSheet(qss.readAll());
                  qss.close();
                }

                menu->move(QCursor::pos());
                menu->show();
              }));

  return true;
}

bool NativeContextMenuHandler::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                    CefRefPtr<CefContextMenuParams> params, int command_id,
                                                    EventFlags event_flags) {
  return true;
}

void NativeContextMenuHandler::OnContextMenuDismissed(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) {}

//================================================================
// class DGCefView
//
DGCefView::DGCefView(QWidget* parrent) : QWidget(parrent) {
  setAttribute(Qt::WA_NativeWindow);
  setAcceptDrops(false);
  setFocusPolicy(Qt::NoFocus);

#if defined(OS_LINUX)
  window_ = 0;
  x_display_ = 0;
  winId();

  Display* display;
  Window window;
  int depth;
  int screen;
  XColor color, dummy;

  display = XOpenDisplay(NULL);
  screen = DefaultScreen(display);
  depth = DefaultDepth(display, screen);
  XSetWindowAttributes attributes;
  attributes.background_pixel = WhitePixel(display, screen);
  attributes.event_mask = VisibilityChangeMask | ExposureMask;
  window_ = (uint64_t)XCreateWindow(display, XDefaultRootWindow(display), 0, 0, size().width(), size().height(), 0,
                                    XDefaultDepth(display, 0), InputOutput, CopyFromParent, CWBackPixel | CWEventMask,
                                    &attributes);
  // window_ = (uint64_t)XCreateWindow(display, XDefaultRootWindow(display), 0, 0, size().width(), size().height(), 0,
  //                                   XDefaultDepth(display, 0), InputOutput, CopyFromParent, 0, &attributes);

  // GC gc = XCreateGC(display, window_, 0, NULL);
  // Colormap colormap = DefaultColormap(display, 0);
  // XParseColor(display, colormap, "#00FF00", &color);
  // XAllocColor(display, colormap, &color);
  // XSetBackground(display, gc, color.pixel);

  XReparentWindow(display, window_, winId(), 0, 0);
  XMapWindow(display, window_);
  XFlush(display);
  x_display_ = (uint64_t)display;

  std::string title_2 = "DGCefView";
  Atom Atom_name = XInternAtom(display, "_NET_WM_NAME", false);
  Atom Atom_utf_type = XInternAtom(display, "UTF8_STRING", false);
  XChangeProperty(display, (XID)window_, Atom_name, Atom_utf_type, 8, PropModeReplace,
                  reinterpret_cast<const unsigned char*>(title_2.c_str()), title_2.size());

  long pid = getpid();
  Atom Atom_Pid = XInternAtom(display, "_NET_WM_PID", false);
  XChangeProperty(display, (XID)window_, Atom_Pid, XA_CARDINAL, 32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(&pid), 1);
#endif
}

DGCefView::~DGCefView() {
#if defined(OS_LINUX)
  Display* display = (Display*)x_display_;
  XCloseDisplay(display);
#endif
}

void DGCefView::Close(bool force) {
#if defined(OS_MAC)
  client::CefWidget::Close(force);
#else  // OS_MAC
  CefWidget::Close(force);
#if !defined(OS_MACOSX)
  CefDoMessageLoopWork();
#endif
#endif  // OS_MAC
  // 回收句柄导致cef崩溃
  // Display* display = (Display*)x_display_;
  // XReparentWindow(display, window_, XDefaultRootWindow(display), 0, 0);
  CefDoMessageLoopWork();
}

void DGCefView::OnBrowserWindowDestroyed() {
  qDebug() << "&&&  OnBrowserWindowDestroyed";
  CefWidget::OnBrowserWindowDestroyed();
#if defined(OS_LINUX) || defined(OS_WIN)
  if (destroy_callback_) destroy_callback_();
#endif
}

#if defined(OS_LINUX) || defined(OS_WIN)
void DGCefView::SetDestroyCallBack(const std::function<void()>& call_back) { destroy_callback_ = call_back; }
#endif

#if defined(OS_LINUX)
void SetXWindowVisible(XDisplay* xdisplay, ::Window xwindow, bool visible) {
  CHECK(xdisplay != 0);

  // Retrieve the atoms required by the below XChangeProperty call.
  const char* kAtoms[] = {"_NET_WM_STATE", "ATOM", "_NET_WM_STATE_HIDDEN"};
  Atom atoms[3];
  int result = XInternAtoms(xdisplay, const_cast<char**>(kAtoms), 3, false, atoms);
  if (!result) NOTREACHED();

  if (!visible) {
    // Set the hidden property state value.
    cef_base::scoped_ptr<Atom[]> data(new Atom[1]);
    data[0] = atoms[2];

    XChangeProperty(xdisplay, xwindow,
                    atoms[0],  // name
                    atoms[1],  // type
                    32,        // size in bits of items in 'value'
                    PropModeReplace, reinterpret_cast<const unsigned char*>(data.get()),
                    1);  // num items
  } else {
    // Set an empty array of property state values.
    XChangeProperty(xdisplay, xwindow,
                    atoms[0],  // name
                    atoms[1],  // type
                    32,        // size in bits of items in 'value'
                    PropModeReplace, NULL,
                    0);  // num items
  }
}
#endif

bool DGCefView::event(QEvent* event) {
  Display* display = (Display*)x_display_;
  switch (event->type()) {
    case QEvent::ParentAboutToChange: {
#if defined(OS_LINUX)
      // XReparentWindow(x_display_, window_, XDefaultRootWindow(display), 0, 0);
      // XFlush(x_display_);
#endif
    } break;
    case QEvent::ParentChange: {
      XReparentWindow(display, window_, winId(), 0, 0);
      // XMapWindow(display, window_);
      XFlush(display);
    } break;
    default:
      break;
  }

  return QWidget::event(event);
}

void DGCefView::dragEnterEvent(QDragEnterEvent* event) { event->acceptProposedAction(); }

void DGCefView::dropEvent(QDropEvent* event) { emit onDropEvent(event); }

#if defined(OS_LINUX)
uint64_t DGCefView::GetXdisplay() { return x_display_; }

void DGCefView::nativeResize(int w, int h) {
  if (x_display_ && window_ && GetBrowser() && GetBrowser()->GetHost()) {
    CefWidget::resize(cefSize());
    XWindowChanges changes = {0};
    changes.x = 0;
    changes.y = 0;
    changes.width = w;
    changes.height = h;
    XConfigureWindow((XDisplay*)x_display_, window_, CWX | CWY | CWHeight | CWWidth, &changes);
    XFlush((XDisplay*)x_display_);
    auto browser_host = GetBrowser()->GetHost();
    browser_host->NotifyMoveOrResizeStarted();
  } else {
    QTimer::singleShot(0, this, [=]() { nativeResize(cefSize().width, cefSize().height); });
  }
}

void DGCefView::forceUpdateCef() { nativeResize(size().width(), size().height()); }

#endif  // OS_LINUX

ClientWindowHandle DGCefView::GetHandle() {
#if defined(OS_WIN) || defined(OS_MACOSX)
  return (ClientWindowHandle)winId();
#elif defined(OS_LINUX)
  return (ClientWindowHandle)window_;
#endif
}

CefSize DGCefView::cefSize() {
  double dpi = 96;
  double scale = 1.0;
#if defined(OS_WIN)
  dpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSX);
  scale = dpi / 96;
  if (scale < 1.5)
    scale = 1;
  else if (scale < 2.5)
    scale = 2;
  else
    scale = 3;
#else
  scale = 1.0;
#endif

  auto dpiX = size().width() * scale;
  auto dpiY = size().height() * scale;

  return CefSize(dpiX, dpiY);
}

void DGCefView::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);

  nativeResize(cefSize().width, cefSize().height);
}

void DGCefView::paintEvent(QPaintEvent* event) {
  QWidget::paintEvent(event);
#if defined(OS_LINUX)
  XFlush((XDisplay*)x_display_);
#endif  // OS_MAC
}

void DGCefView::OnSetLoadingState(bool isLoading, bool canGoBack, bool canGoForward) {
  emit loadingStateChanged(isLoading, canGoBack, canGoForward);
}

void DGCefView::OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const std::string& url) {
  if (biz::GetSchemeServiceStd()->MatchToHandle((void*)browser->GetIdentifier(), url, "", false)) {
    return;
  }
  utility::OsShell::ShellOpenFile(url);
}

void DGCefView::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
                               bool is_redirect, bool& cancel) {
  std::string lower_url = request->GetURL();
  if (can_match_ && biz::GetSchemeServiceStd()->MatchToHandle((void*)browser->GetIdentifier(), lower_url, "", false)) {
    cancel = true;  // invoker return this param.
    return;
  }
}

bool DGCefView::OnPreKeyEvent(const CefKeyEvent& event, CefEventHandle os_event, bool* is_keyboard_shortcut) {
#if defined(OS_WIN)
  int escape_key = 65537;
  int f11_key = 128;
#elif defined(OS_LINUX)
  int escape_key = 9;
  int f11_key = 128;
#else
  int escape_key = 9;
  int f11_key = 128;
#endif

  if (event.native_key_code == escape_key || event.native_key_code == f11_key) {
    emit escapePrePress();
  }
  return false;
}

#if defined(OS_MAC) || defined(OS_MACOSX)
bool DGCefView::OnDragEnter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> dragData,
                            CefDragHandler::DragOperationsMask mask) {
  std::vector<CefString> files;
  if (dragData->GetFileNames(files)) {
    std::vector<std::string> filenames;
    for (CefString file : files) {
      filenames.push_back(file.ToString());
    }

    emit onDropEvent2(filenames);
  }
  return false;
}
#endif  // OS_MAC

CefRefPtr<CefDialogHandler> DGCefView::GetDialogHandler() { return new NativeDialogHandler; }

CefRefPtr<CefContextMenuHandler> DGCefView::GetContextMenuHandler() {
  // return new NativeContextMenuHandler;
  return nullptr;
}

void DGCefView::SetNativeFocus() {
  if (GetBrowser() && GetBrowser()->GetHost()) {
    XSetInputFocus((Display*)x_display_, GetBrowser()->GetHost()->GetWindowHandle(), RevertToParent, CurrentTime);
  }
}

void DGCefView::GlobalSetNativeFocus(QWidget* w) {
  Display* display = XOpenDisplay(NULL);
  XSetInputFocus(display, None, RevertToParent, CurrentTime);
}

void DGCefView::OnSetTitle(const std::string& title) { emit titleChanged(title); }

void DGCefView::OnSetFullscreen(bool fullscreen) { emit requestFullscreenMode(fullscreen); }

void DGCefView::OnSetFavicon(const std::string& image) {
  emit faviconChanged(image);
}

}  // namespace client_ding
