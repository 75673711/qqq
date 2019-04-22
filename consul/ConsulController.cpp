#include "ConsulController.h"

#include "curl/curl.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

#include <stdio.h>

#include <QDebug>

namespace funnyconsul {

using namespace  std;

ConsulController::ConsulController()
{
    Init();
}

ConsulController::~ConsulController()
{
    Uninit();
}

bool ConsulController::Init()
{
#ifdef Q_OS_WIN32
    CURLcode r = curl_global_init(CURL_GLOBAL_WIN32 | CURL_GLOBAL_ALL);
#endif

#ifdef Q_OS_MAC
#endif

#ifdef Q_OS_LINUX
    CURLcode r = curl_global_init(CURL_GLOBAL_ALL);
#endif

    return (r ==CURLE_OK);
}

void ConsulController::Uninit()
{
    curl_global_cleanup();
}

// size   单位字节          nitems   单位个数         （比如  char   单个1字节   一共64K个）
static size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    CCRequest* ptr_request = static_cast<CCRequest*>(userdata);
    if (ptr_request == nullptr || ptr_request->body.empty())
    {
        return 0;
    }
    else
    {
        size_t max = size * nitems;
        if (max < ptr_request->body.length())
        {
            // 最大64K    分次写入   需要在response中记录当前pos
//            size_t pos = 0;
//            {
//                size_t once = min(max, ptr_request->body.length() - pos);
//                memcpy(buffer + pos, ptr_request->body.c_str() + pos, ptr_request->body.length());

//                pos += once;
//            }
            // return once;
        }
        else
        {
            memcpy(buffer, ptr_request->body.c_str(), ptr_request->body.length());
            return ptr_request->body.length();
        }
    }
}

static size_t header_callback(char *buffer, size_t size,
                              size_t nitems, void *userdata)
{
    CCResponse* ptr_response = static_cast<CCResponse*>(userdata);
    if (ptr_response != nullptr)
    {
        ptr_response->header += std::string(buffer, size * nitems);
    }

  return nitems * size;
}

static size_t write_callback(char *buffer, size_t size, size_t nmemb, void *userdata)
{
    CCResponse* ptr_response = static_cast<CCResponse*>(userdata);
    if (ptr_response != nullptr)
    {
        ptr_response->body += std::string(buffer, size * nmemb);
    }

  return nmemb * size;
}

// todo: 如何终止    超时时长    异步调用
ConsulController::CCStatus ConsulController::DoGet(CCResponse &response, const CCRequest &request)
{
    if (request.url.empty())
    {
        return CCSTATUS__INVALIDPARAM;
    }

    response.header = string();
    response.body = string();

    CURL* ptr_curl = curl_easy_init();
    if (ptr_curl != nullptr)
    {
        CURLcode res;
        curl_easy_setopt(ptr_curl, CURLOPT_URL, QUrl(request.url.c_str()).toEncoded().constData());
        curl_easy_setopt(ptr_curl, CURLOPT_HTTPGET, 1L);

        curl_easy_setopt(ptr_curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(ptr_curl, CURLOPT_WRITEDATA, &response);

        curl_easy_setopt(ptr_curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(ptr_curl, CURLOPT_HEADERDATA, &response);

        res = curl_easy_perform(ptr_curl);
        curl_easy_cleanup(ptr_curl);

        if (res != CURLE_OK)
        {
            return CCSTATUS__GETFAILD;
        }

        return CCSTATUS_OK;
    }

    return CCSTATUS_INITFAILED;
}

ConsulController::CCStatus ConsulController::DoPost(CCResponse& response, const CCRequest &request)
{
    if (request.url.empty())
    {
        return CCSTATUS__INVALIDPARAM;
    }

    response.header = string();
    response.body = string();

    CURL* ptr_curl = curl_easy_init();
    if (ptr_curl != nullptr)
    {
        CURLcode res;
        curl_easy_setopt(ptr_curl, CURLOPT_URL, QUrl(request.url.c_str()).toEncoded().constData());
        curl_easy_setopt(ptr_curl, CURLOPT_POST, 1L);

        curl_easy_setopt(ptr_curl, CURLOPT_POSTFIELDSIZE, request.body.size());
        curl_easy_setopt(ptr_curl, CURLOPT_POSTFIELDS, request.body.c_str());

        curl_easy_setopt(ptr_curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(ptr_curl, CURLOPT_WRITEDATA, &response);

        curl_easy_setopt(ptr_curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(ptr_curl, CURLOPT_HEADERDATA, &response);

        res = curl_easy_perform(ptr_curl);
        curl_easy_cleanup(ptr_curl);

        if (res != CURLE_OK)
        {
            return CCSTATUS__GETFAILD;
        }

        return CCSTATUS_OK;
    }

    return CCSTATUS_INITFAILED;
}

ConsulController::CCStatus ConsulController::DoPut(CCResponse& response, const CCRequest& request)
{
    if (request.url.empty())
    {
        return CCSTATUS__INVALIDPARAM;
    }

    response.header = string();
    response.body = string();

    CURL* ptr_curl = curl_easy_init();
    if (ptr_curl != nullptr)
    {
        CURLcode res;
        curl_easy_setopt(ptr_curl, CURLOPT_URL, QUrl(request.url.c_str()).toEncoded().constData());
        curl_easy_setopt(ptr_curl, CURLOPT_UPLOAD, 1L);

        curl_easy_setopt(ptr_curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(ptr_curl, CURLOPT_READDATA, &request);
        curl_easy_setopt(ptr_curl, CURLOPT_INFILESIZE, request.body.size());

        curl_easy_setopt(ptr_curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(ptr_curl, CURLOPT_WRITEDATA, &response);

        curl_easy_setopt(ptr_curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(ptr_curl, CURLOPT_HEADERDATA, &response);

        res = curl_easy_perform(ptr_curl);
        curl_easy_cleanup(ptr_curl);

        if (res != CURLE_OK)
        {
            return CCSTATUS__GETFAILD;
        }

        return CCSTATUS_OK;
    }

    return CCSTATUS_INITFAILED;
}

/**************************************************************************************/

bool ConsulController::JsonToService(CCService& service, const QJsonObject& json_obj)
{
    if (json_obj.isEmpty())
    {
        service = CCService();
        return true;
    }

    if (json_obj.contains("Name"))
    {
        service.Name = json_obj.value("Name").toString().toStdString();
    }
    if (json_obj.contains("ID"))
    {
        service.ID = json_obj.value("ID").toString().toStdString();
    }
    if (json_obj.contains("Tags"))
    {
        QJsonArray tags = json_obj.value("Tags").toArray();
        for (auto it : tags)
        {
           service.Tags.push_back(it.toString().toStdString());
        }
    }
    if (json_obj.contains("Address"))
    {
        service.Address = json_obj.value("Address").toString().toStdString();
    }
    if (json_obj.contains("Meta"))
    {
        QJsonObject meta = json_obj.value("Name").toObject();
        QJsonObject::iterator  iter = meta.begin();
        while (iter != meta.end())
        {
            string key = iter.key().toStdString();
            string value = iter.value().toString().toStdString();
            service.Meta[key] = value;
        }
    }

    return true;
}

bool ConsulController::ServiceToJson(QJsonObject& json_obj, const CCService& service)
{
    if (service.IsEmpty())
    {
        return false;
    }

    json_obj["Name"] = QString::fromStdString(service.Name);

    if (!service.ID.empty())
    {
        json_obj["ID"] = QString::fromStdString(service.ID);
    }

    if (service.Tags.size() > 0)
    {
        QJsonArray tags;
        for (const auto& it : service.Tags)
        {
            tags.append(QString::fromStdString(it));
        }
        json_obj["Tags"] = tags;
    }

    if (!service.Address.empty())
    {
        json_obj["Address"] = QString::fromStdString(service.Address);
    }

    if (service.Meta.size() > 0)
    {
        std::map<std::string, std::string>::const_iterator iter = service.Meta.begin();
        QJsonObject meta;
        while (iter != service.Meta.end())
        {
            QString key = QString::fromStdString(iter->first);
            QString value = QString::fromStdString(iter->second);
            meta[key] = value;
            ++iter;
        }

        json_obj["Meta"] = meta;
    }

    if (service.Port != 0)
    {
        json_obj["Port"] = service.Port;
    }

    return true;
}

ConsulController::CCCommandStatus ConsulController::GetAllService(std::vector<CCService>& service_vec)
{
    service_vec.clear();

    funnyconsul::CCResponse response;
    funnyconsul::CCRequest request;
    request.url = "http://127.0.0.1:8500/v1/agent/services";
    if (DoGet(response, request) == CCSTATUS_OK)
    {
        QJsonDocument doc = QJsonDocument::fromJson(response.body.c_str());
        if (doc.isObject())
        {
            QJsonObject obj = doc.object();

            QJsonObject::iterator  iter = obj.begin();
            while (iter != obj.end())
            {
                CCService one;
                if (JsonToService(one, iter.value().toObject()))
                {
                    service_vec.push_back(one);
                }
                else
                {
                    Q_ASSERT(false);
                }

                ++iter;
            }

            return CCCommandStatus_OK;
        }
        else
        {
            qDebug() << "GetAllService  back is not object";
        }
    }

    return CCCommandStatus_FAILED;
}

// http://127.0.0.1:8500/v1/agent/service/[service_id]
ConsulController::CCCommandStatus ConsulController::GetOneService(CCService& service, const std::string& service_id)
{
    funnyconsul::CCResponse response;
    funnyconsul::CCRequest request;
    request.url = "http://127.0.0.1:8500/v1/agent/service/" + service_id;
    if (DoGet(response, request) == CCSTATUS_OK)
    {
        QJsonDocument doc = QJsonDocument::fromJson(response.body.c_str());
        if (doc.isObject())
        {
            QJsonObject obj = doc.object();

            if (JsonToService(service, obj))
            {
                return CCCommandStatus_OK;
            }
            else
            {
                Q_ASSERT(false);
            }
        }
    }

    return CCCommandStatus_FAILED;
}

ConsulController::CCCommandStatus ConsulController::RegisterService(const CCService& service)
{
    QJsonObject obj;
    if (!ServiceToJson(obj, service))
    {
        return CCCommandStatus_FAILED;
    }

    QJsonDocument doc(obj);

    funnyconsul::CCResponse response;
    funnyconsul::CCRequest request;
    request.url = "http://127.0.0.1:8500/v1/agent/service/register";
    request.body = doc.toJson().toStdString();
    if (DoPut(response, request) == CCSTATUS_OK)
    {
        return CCCommandStatus_OK;
    }

    return CCCommandStatus_FAILED;
}

ConsulController::CCCommandStatus ConsulController::DeregisterService(const std::string& service_id)
{
    funnyconsul::CCResponse response;
    funnyconsul::CCRequest request;
    request.url = "http://127.0.0.1:8500/v1/agent/service/deregister/" + service_id;
    if (DoPut(response, request) == CCSTATUS_OK)
    {
        return CCCommandStatus_OK;
    }

    return CCCommandStatus_FAILED;
}

}
