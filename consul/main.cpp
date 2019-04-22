#include "globalinit.h"

#include <QCoreApplication>

#include "controller/client/consul/ConsulController.h"

#include <QThread>

#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);

    Init(&app);

//    std::string service_id = "funnyservice";

//    funnyconsul::CCService one;
//    one.Name = service_id;
//    one.ID = "blabla _id";
//    qDebug() << "RegisterService: " << funnyconsul::ConsulController::GetInstance().RegisterService(one);
//    qDebug() << "done";

//    funnyconsul::CCService get_one;
//    qDebug() << "GetOneService: " << funnyconsul::ConsulController::GetInstance().GetOneService(get_one, "blabla _id");
//    qDebug() << "done " << get_one.ID.c_str();

//    qDebug() << "DeregisterService: " << funnyconsul::ConsulController::GetInstance().DeregisterService("blabla _id");
//    qDebug() << "done";

    return app.exec();
}
