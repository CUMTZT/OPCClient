//
// Created by 张腾 on 25-3-10.
//
#include "AscendingMessageHandler.h"
#include "Logger.h"
#include "OPCClientManager.h"
#include <QDebug>
#include <thread>

std::string dump_headers(const httplib::Headers &headers) {
    std::string s;
    char buf[BUFSIZ];
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
        s += buf;
    }
    return s;
}

std::string log(const httplib::Request &req, const httplib::Response &res) {
    std::string s;
    char buf[BUFSIZ];
    s += "================================\n";
    snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
             req.version.c_str(), req.path.c_str());
    s += buf;
    std::string query;
    for (auto it = req.params.begin(); it != req.params.end(); ++it) {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%c%s=%s",
                 (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
                 x.second.c_str());
        query += buf;
    }
    snprintf(buf, sizeof(buf), "%s\n", query.c_str());
    s += buf;
    s += dump_headers(req.headers);
    s += "--------------------------------\n";
    snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
    s += buf;
    s += dump_headers(res.headers);
    s += "\n";
    if (!res.body.empty()) { s += res.body; }
    s += "\n";
    return s;
}

AscendingMessageHandler::AscendingMessageHandler(QObject *parent): QObject(parent) {
    initServer();
}

AscendingMessageHandler::~AscendingMessageHandler() {
    mpServer->stop();
    mpThread->join();
    delete mpServer;
    delete mpThread;
}

void AscendingMessageHandler::setPort(int port) {
    if (nullptr != mpServer) {
        mpServer->stop();
        delete mpServer;
        mpServer = nullptr;
    }
    if (nullptr != mpThread) {
        delete mpThread;
        mpThread = nullptr;
    }
    initServer();
}

int AscendingMessageHandler::port() const {
    return mPort;
}

void AscendingMessageHandler::initServer() {
    mpServer = new httplib::Server();
    if (!mpServer->is_valid()) {
        LogErr("Http服务端开启失败");
    }

    mpServer->Get("/update", [this](const httplib::Request &req, httplib::Response &res) {
        std::string machine = req.get_param_value("machine");
        std::string code = req.get_param_value("code");
        std::string value = req.get_param_value("value");
        try {
            OPCClientManagerIns->onSetDataValue(machine, {code, value});
            res.status = 200;
            res.set_content("Success!\n", "text/plain");
        } catch (std::exception &e) {
            std::rethrow_exception(std::current_exception());
        }
    });

    mpServer->set_exception_handler([](const httplib::Request & /*req*/, httplib::Response &res,const std::exception_ptr& ep) {
        const char *fmt = "<p>Exception Occured: <span style='color:red;'>%s</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, std::strerror(errno));
        res.status = 500;
        res.set_content(buf, "text/html");
    });

    mpServer->set_error_handler([](const httplib::Request & /*req*/, httplib::Response &res) {
        const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html");
    });

    mpServer->set_logger([](const httplib::Request &req, const httplib::Response &res) {
        LogInfo("{}", log(req, res).c_str());
    });

    mpThread = new std::thread([this]() {
        mpServer->listen("0.0.0.0", mPort);
    });
    mpThread->detach();
}
