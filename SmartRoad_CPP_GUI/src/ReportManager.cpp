#include "ReportManager.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

ReportManager::~ReportManager() { while (head) { Node *n = head; head = head->next; delete n; } }
int ReportManager::add(RoadReport r) {
    r.id = nextId++;
    if (!r.createdAt.isValid()) r.createdAt = QDateTime::currentDateTime();
    Node *n = new Node(r);
    if (!head) head = n; else { Node *p = head; while (p->next) p = p->next; p->next = n; }
    if (r.status == "Pending") pendingQueue.enqueue(r.id);
    recentStack.push(r.id);
    return r.id;
}
QVector<RoadReport> ReportManager::all() const {
    QVector<RoadReport> v; for (Node *p = head; p; p = p->next) v.push_back(p->report); return v;
}
bool ReportManager::find(int id, RoadReport &out) const {
    for (Node *p = head; p; p = p->next) if (p->report.id == id) { out = p->report; return true; } return false;
}
bool ReportManager::updateStatus(int id, const QString &status) {
    for (Node *p = head; p; p = p->next) if (p->report.id == id) { p->report.status = status; if (status == "Pending") pendingQueue.enqueue(id); return true; } return false;
}
bool ReportManager::remove(int id) {
    Node *prev = nullptr, *p = head;
    while (p) { if (p->report.id == id) { if (prev) prev->next = p->next; else head = p->next; delete p; return true; } prev = p; p = p->next; }
    return false;
}
int ReportManager::processNextPending() {
    while (!pendingQueue.isEmpty()) { int id = pendingQueue.dequeue(); RoadReport r; if (find(id, r) && r.status == "Pending") { updateStatus(id, "In Progress"); return id; } } return -1;
}
int ReportManager::mostRecentId() const { return recentStack.isEmpty() ? -1 : recentStack.top(); }
int ReportManager::total() const { int n = 0; for (Node *p = head; p; p = p->next) ++n; return n; }
bool ReportManager::save(const QString &path, QString &error) const {
    QJsonArray arr;
    for (const auto &r : all()) { QJsonObject o; o["id"] = r.id; o["place"] = r.place; o["severity"] = r.severity; o["status"] = r.status; o["description"] = r.description; o["imagePath"] = r.imagePath; o["createdAt"] = r.createdAt.toString(Qt::ISODate); o["brightness"] = r.brightness; o["edgePercent"] = r.edgePercent; arr.append(o); }
    QFile f(path); if (!f.open(QIODevice::WriteOnly)) { error = f.errorString(); return false; }
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented)); return true;
}
bool ReportManager::load(const QString &path, QString &error) {
    QFile f(path); if (!f.exists()) return true; if (!f.open(QIODevice::ReadOnly)) { error = f.errorString(); return false; }
    QJsonParseError pe; auto doc = QJsonDocument::fromJson(f.readAll(), &pe); if (pe.error != QJsonParseError::NoError || !doc.isArray()) { error = "Saved report file is invalid JSON."; return false; }
    while (head) { Node *n = head; head = head->next; delete n; } pendingQueue.clear(); recentStack.clear(); nextId = 1;
    for (const auto val : doc.array()) { auto o = val.toObject(); RoadReport r; r.id = o["id"].toInt(); r.place = o["place"].toString(); r.severity = o["severity"].toString(); r.status = o["status"].toString(); r.description = o["description"].toString(); r.imagePath = o["imagePath"].toString(); r.createdAt = QDateTime::fromString(o["createdAt"].toString(), Qt::ISODate); r.brightness = o["brightness"].toDouble(-1); r.edgePercent = o["edgePercent"].toDouble(-1); Node *n = new Node(r); if (!head) head = n; else { Node *p = head; while (p->next) p = p->next; p->next = n; } nextId = qMax(nextId, r.id + 1); if (r.status == "Pending") pendingQueue.enqueue(r.id); recentStack.push(r.id); }
    return true;
}
