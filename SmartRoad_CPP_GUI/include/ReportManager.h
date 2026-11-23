#pragma once
#include "RoadReport.h"
#include <QVector>
#include <QQueue>
#include <QStack>
#include <QString>

// DSA: custom singly linked list is the primary report store.
class ReportManager {
    struct Node { RoadReport report; Node *next = nullptr; explicit Node(const RoadReport &r) : report(r) {} };
    Node *head = nullptr;
    int nextId = 1;
    QQueue<int> pendingQueue; // FIFO pending workflow
    QStack<int> recentStack;  // LIFO recent-report history
public:
    ReportManager() = default;
    ~ReportManager();
    ReportManager(const ReportManager&) = delete;
    ReportManager& operator=(const ReportManager&) = delete;
    int add(RoadReport report);
    QVector<RoadReport> all() const;
    bool find(int id, RoadReport &out) const;
    bool updateStatus(int id, const QString &status);
    bool remove(int id);
    int processNextPending();
    int mostRecentId() const;
    int total() const;
    bool save(const QString &path, QString &error) const;
    bool load(const QString &path, QString &error);
};
