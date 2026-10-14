#include "RoadReport.h"
#include "ReportManager.h"
#include "ImageManager.h"
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStatusBar>
#include <QFrame>
#include <QScrollArea>

class SmartRoadWindow : public QMainWindow {
    ReportManager manager;
    QString selectedImage;
    QString dataDir;
    QLabel *imagePreview = nullptr, *analysisLabel = nullptr, *summaryLabel = nullptr;
    QLineEdit *placeEdit = nullptr;
    QComboBox *severityBox = nullptr, *statusBox = nullptr;
    QTextEdit *descriptionEdit = nullptr;
    QTableWidget *table = nullptr;
    QTabWidget *tabWidget = nullptr;
    QLineEdit *searchEdit = nullptr;
    QLabel *totalValue = nullptr, *severeValue = nullptr, *moderateValue = nullptr, *goodValue = nullptr;

    QString dataFile() const { return dataDir + "/reports.json"; }
    void saveQuietly() { QString e; if (!manager.save(dataFile(), e)) statusBar()->showMessage("Save failed: " + e, 5000); }
    QLabel *metric(const QString &title, QLabel **value, const QString &accent) {
        auto *card = new QFrame; card->setObjectName("metricCard"); card->setStyleSheet("QFrame#metricCard{background:white;border:1px solid #dce6f2;border-left:5px solid " + accent + ";border-radius:10px;}");
        auto *l = new QVBoxLayout(card); auto *t = new QLabel(title); t->setStyleSheet("color:#52647a;font-size:12px;font-weight:600;"); *value = new QLabel("0"); (*value)->setStyleSheet("font-size:25px;font-weight:700;color:#16345c;"); l->addWidget(t); l->addWidget(*value); return new QLabel;
    }
    QWidget *buildDashboard() {
        auto *page = new QWidget; auto *root = new QVBoxLayout(page); root->setSpacing(14);
        auto *cards = new QHBoxLayout; cards->setSpacing(12);
        auto addCard = [&](const QString &title, QLabel **val, const QString &color) { auto *frame = new QFrame; frame->setObjectName("metricCard"); frame->setStyleSheet("QFrame#metricCard{background:white;border:1px solid #dce6f2;border-left:5px solid " + color + ";border-radius:10px;}"); auto *v = new QVBoxLayout(frame); auto *t = new QLabel(title); t->setStyleSheet("color:#52647a;font-weight:600;"); *val = new QLabel("0"); (*val)->setStyleSheet("font-size:26px;font-weight:700;color:#16345c;"); v->addWidget(t); v->addWidget(*val); cards->addWidget(frame); };
        addCard("TOTAL REPORTS", &totalValue, "#3182f6"); addCard("SEVERE", &severeValue, "#e74c3c"); addCard("MODERATE", &moderateValue, "#f39c12"); addCard("GOOD / MINOR", &goodValue, "#19a974"); root->addLayout(cards);
        auto *intro = new QFrame; intro->setStyleSheet("QFrame{background:white;border:1px solid #dce6f2;border-radius:10px;}"); auto *il = new QVBoxLayout(intro);
        auto *title = new QLabel("Road Condition Monitoring"); title->setStyleSheet("font-size:20px;font-weight:700;color:#17365e;border:0;");
        auto *desc = new QLabel("Upload a road photo, enter a place name (not coordinates), review basic OpenCV image statistics, and manage reports using DSA."); desc->setWordWrap(true); desc->setStyleSheet("color:#53677f;border:0;font-size:13px;");
        auto *actions = new QHBoxLayout; auto *add = new QPushButton("＋  Add Road Report"); auto *map = new QPushButton("⌖  Open Map Search"); add->setObjectName("primaryButton"); actions->addWidget(add); actions->addWidget(map); actions->addStretch();
        connect(add, &QPushButton::clicked, this, [this](){ if (tabWidget) tabWidget->setCurrentIndex(1); });
        connect(map, &QPushButton::clicked, this, [this](){ openMapForPlace(placeEdit ? placeEdit->text() : QString()); });
        il->addWidget(title); il->addWidget(desc); il->addLayout(actions); root->addWidget(intro);
        auto *recentTitle = new QLabel("Recent Road Reports"); recentTitle->setStyleSheet("font-size:16px;font-weight:700;color:#17365e;"); root->addWidget(recentTitle);
        table = new QTableWidget(0, 6); table->setHorizontalHeaderLabels({"ID", "Place", "Severity", "Status", "Date", "Image"}); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setAlternatingRowColors(true); root->addWidget(table, 1);
        return page;
    }
    QWidget *buildAddReport() {
        auto *page = new QWidget; auto *root = new QVBoxLayout(page); auto *title = new QLabel("Submit a Road Condition Report"); title->setStyleSheet("font-size:20px;font-weight:700;color:#17365e;"); root->addWidget(title);
        auto *body = new QHBoxLayout; auto *left = new QVBoxLayout; auto *photoGroup = new QGroupBox("Road Photo"); auto *pg = new QVBoxLayout(photoGroup);
        imagePreview = new QLabel("No image selected\n\nChoose a JPG or PNG road photo"); imagePreview->setAlignment(Qt::AlignCenter); imagePreview->setMinimumSize(300, 210); imagePreview->setStyleSheet("background:#f3f7fc;border:2px dashed #9bbbe8;border-radius:8px;color:#60758f;padding:12px;"); imagePreview->setScaledContents(false);
        auto *choose = new QPushButton("Choose Road Photo…"); auto *analyze = new QPushButton("Analyze with OpenCV"); pg->addWidget(imagePreview, 1); pg->addWidget(choose); pg->addWidget(analyze); left->addWidget(photoGroup, 3);
        auto *analysisGroup = new QGroupBox("Image Analysis (basic statistics)"); auto *ag = new QVBoxLayout(analysisGroup); analysisLabel = new QLabel("Select a photo, then run analysis. This does not automatically confirm potholes."); analysisLabel->setWordWrap(true); ag->addWidget(analysisLabel); left->addWidget(analysisGroup, 1);
        auto *formGroup = new QGroupBox("Report Details"); auto *form = new QFormLayout(formGroup); placeEdit = new QLineEdit; placeEdit->setPlaceholderText("e.g. MG Road, Dehradun"); severityBox = new QComboBox; severityBox->addItems({"Severe", "Moderate", "Good / Minor"}); statusBox = new QComboBox; statusBox->addItems({"Pending", "In Progress", "Resolved"}); descriptionEdit = new QTextEdit; descriptionEdit->setPlaceholderText("Describe the road issue (optional)…"); descriptionEdit->setMaximumHeight(110);
        auto *locationNote = new QLabel("Enter a recognizable place/address. Latitude and longitude are not required."); locationNote->setWordWrap(true); locationNote->setStyleSheet("color:#60758f;font-size:11px;");
        form->addRow("Place / Location *", placeEdit); form->addRow("", locationNote); form->addRow("Condition Severity *", severityBox); form->addRow("Workflow Status", statusBox); form->addRow("Description", descriptionEdit);
        auto *formCol = new QVBoxLayout; formCol->addWidget(formGroup); auto *mapBtn = new QPushButton("Open this place in OpenStreetMap"); auto *submit = new QPushButton("Submit Road Report"); submit->setObjectName("primaryButton"); formCol->addWidget(mapBtn); formCol->addStretch(); formCol->addWidget(submit);
        body->addLayout(left, 1); body->addLayout(formCol, 1); root->addLayout(body, 1);
        connect(choose, &QPushButton::clicked, this, [this](){ QString p = QFileDialog::getOpenFileName(this, "Choose road photo", QDir::homePath(), "Images (*.png *.jpg *.jpeg *.bmp)"); if (p.isEmpty()) return; selectedImage = p; QPixmap pix(p); if (pix.isNull()) { QMessageBox::warning(this, "Image error", "Could not open this image."); return; } imagePreview->setPixmap(pix.scaled(imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)); analysisLabel->setText("Photo selected. Click Analyze with OpenCV to calculate basic image statistics."); });
        connect(analyze, &QPushButton::clicked, this, [this](){ if (selectedImage.isEmpty()) { QMessageBox::information(this, "Choose photo", "Please choose a road photo first."); return; } double b=0,e=0; QString err; if (!ImageManager::analyze(selectedImage,b,e,err)) { QMessageBox::warning(this,"OpenCV analysis",err); return; } analysisLabel->setText(QString("OpenCV analysis complete\nAverage brightness: %1 / 255\nEdge pixels: %2%\n\nThese are image statistics, not an automatic pothole classification.").arg(b,0,'f',2).arg(e,0,'f',2)); });
        connect(mapBtn, &QPushButton::clicked, this, [this](){ openMapForPlace(placeEdit->text()); });
        connect(submit, &QPushButton::clicked, this, [this](){ if (placeEdit->text().trimmed().isEmpty()) { QMessageBox::warning(this,"Missing location","Please enter a place name or address."); placeEdit->setFocus(); return; } if (selectedImage.isEmpty()) { QMessageBox::warning(this,"Missing photo","Please upload a road photo."); return; } RoadReport r; r.place=placeEdit->text().trimmed(); r.severity=severityBox->currentText(); r.status=statusBox->currentText(); r.description=descriptionEdit->toPlainText().trimmed(); r.imagePath=selectedImage; double b=-1,e=-1; QString err; if (ImageManager::analyze(selectedImage,b,e,err)) { r.brightness=b; r.edgePercent=e; } int id=manager.add(r); saveQuietly(); refreshTable(); QMessageBox::information(this,"Report submitted",QString("Road report SR%1 saved successfully.").arg(id,4,10,QChar('0'))); placeEdit->clear(); descriptionEdit->clear(); selectedImage.clear(); imagePreview->setPixmap(QPixmap()); imagePreview->setText("No image selected\n\nChoose a JPG or PNG road photo"); analysisLabel->setText("Select a photo, then run analysis. This does not automatically confirm potholes."); });
        return page;
    }
    QWidget *buildReportsPage() {
        auto *page = new QWidget; auto *root = new QVBoxLayout(page); auto *title = new QLabel("All Road Reports"); title->setStyleSheet("font-size:20px;font-weight:700;color:#17365e;"); root->addWidget(title);
        auto *bar = new QHBoxLayout; searchEdit = new QLineEdit; searchEdit->setPlaceholderText("Search by report ID or place…"); auto *search = new QPushButton("Search"); auto *all = new QPushButton("Show All"); auto *process = new QPushButton("Process Next Pending (Queue)"); auto *recent = new QPushButton("Show Latest ID (Stack)"); bar->addWidget(searchEdit,1); bar->addWidget(search); bar->addWidget(all); root->addLayout(bar); auto *dsa = new QHBoxLayout; dsa->addWidget(process); dsa->addWidget(recent); dsa->addStretch(); root->addLayout(dsa);
        auto *t = new QTableWidget(0,6); t->setHorizontalHeaderLabels({"ID","Place","Severity","Status","Created","Image path"}); t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); t->setEditTriggers(QAbstractItemView::NoEditTriggers); t->setSelectionBehavior(QAbstractItemView::SelectRows); t->setAlternatingRowColors(true); root->addWidget(t,1);
        connect(search,&QPushButton::clicked,this,[this,t](){ QString q=searchEdit->text().trimmed(); t->setRowCount(0); for(const auto&r:manager.all()) if(q.isEmpty()||QString::number(r.id).contains(q,Qt::CaseInsensitive)||r.place.contains(q,Qt::CaseInsensitive)) addTableRow(t,r); });
        connect(all,&QPushButton::clicked,this,[this,t](){ t->setRowCount(0); for(const auto&r:manager.all()) addTableRow(t,r); });
        connect(process,&QPushButton::clicked,this,[this,t](){ int id=manager.processNextPending(); if(id<0) QMessageBox::information(this,"Queue","No pending report is available."); else { saveQuietly(); refreshTable(); t->setRowCount(0); for(const auto&r:manager.all()) addTableRow(t,r); QMessageBox::information(this,"Queue",QString("Report SR%1 moved to In Progress.").arg(id,4,10,QChar('0'))); } });
        connect(recent,&QPushButton::clicked,this,[this](){ int id=manager.mostRecentId(); QMessageBox::information(this,"Stack",id<0?"No reports yet.":QString("Most recent report ID: SR%1").arg(id,4,10,QChar('0'))); });
        return page;
    }
    void addTableRow(QTableWidget *t,const RoadReport&r) { int row=t->rowCount(); t->insertRow(row); t->setItem(row,0,new QTableWidgetItem(QString("SR%1").arg(r.id,4,10,QChar('0')))); t->setItem(row,1,new QTableWidgetItem(r.place)); auto *sev=new QTableWidgetItem(r.severity); if(r.severity=="Severe") sev->setForeground(QColor("#d93636")); else if(r.severity=="Moderate") sev->setForeground(QColor("#c77b00")); else sev->setForeground(QColor("#12805c")); t->setItem(row,2,sev); t->setItem(row,3,new QTableWidgetItem(r.status)); t->setItem(row,4,new QTableWidgetItem(r.createdAt.toString("yyyy-MM-dd HH:mm"))); t->setItem(row,5,new QTableWidgetItem(r.imagePath)); }
    void refreshTable() {
        if(table){ table->setRowCount(0); auto reports=manager.all(); for(int i=reports.size()-1;i>=0;--i) addTableRow(table,reports[i]); }
        int severe=0,moderate=0,good=0,pending=0; for(const auto&r:manager.all()){ if(r.severity=="Severe")++severe; else if(r.severity=="Moderate")++moderate; else ++good; if(r.status!="Resolved")++pending; }
        if(totalValue){ totalValue->setText(QString::number(manager.total())); severeValue->setText(QString::number(severe)); moderateValue->setText(QString::number(moderate)); goodValue->setText(QString::number(good)); }
        if(summaryLabel) summaryLabel->setText(QString("Total reports: %1\nSevere: %2\nModerate: %3\nGood / Minor: %4\nUnresolved reports: %5\n\nReports are saved locally in data/reports.json.").arg(manager.total()).arg(severe).arg(moderate).arg(good).arg(pending));
    }
    void openMapForPlace(QString place) { if(place.trimmed().isEmpty()) { bool ok=false; place=QInputDialog::getText(this,"OpenStreetMap","Enter a place name or address (no coordinates needed):",QLineEdit::Normal,"",&ok); if(!ok||place.trimmed().isEmpty()) return; } QUrl url("https://www.openstreetmap.org/search?query="+QString::fromLatin1(QUrl::toPercentEncoding(place.trimmed()))); QDesktopServices::openUrl(url); }
public:
    SmartRoadWindow() {
        setWindowTitle("SmartRoad — Road Condition Monitoring System (C++ / OpenCV)"); resize(1250,800); dataDir=QDir::currentPath()+"/data"; QDir().mkpath(dataDir); QString err; if(!manager.load(dataFile(),err)&&!err.isEmpty()) QMessageBox::warning(this,"Saved reports",err);
        auto *central=new QWidget; auto *outer=new QVBoxLayout(central); outer->setContentsMargins(0,0,0,0); outer->setSpacing(0);
        auto *header=new QFrame; header->setStyleSheet("background:#17365e;"); auto *hl=new QHBoxLayout(header); auto *brand=new QLabel("  SmartRoad  |  Safer Roads, Smarter Tomorrow"); brand->setStyleSheet("color:white;font-size:20px;font-weight:700;padding:12px;"); auto *online=new QLabel("● Local system ready  "); online->setStyleSheet("color:#a7f3d0;font-weight:600;"); hl->addWidget(brand); hl->addStretch(); hl->addWidget(online); outer->addWidget(header);
        auto *tabs=new QTabWidget; tabWidget=tabs; tabs->setDocumentMode(true); tabs->addTab(buildDashboard(),"Dashboard"); tabs->addTab(buildAddReport(),"＋ Add Report"); tabs->addTab(buildReportsPage(),"View / Search Reports"); tabs->addTab(buildAnalytics(),"Analytics & Map"); outer->addWidget(tabs,1); setCentralWidget(central);
        statusBar()->showMessage("C++ | Qt Widgets | OpenCV | Linked List + Queue + Stack | OpenStreetMap");
        connect(tabs,&QTabWidget::currentChanged,this,[this](int){refreshTable();});
        setStyleSheet("QMainWindow{background:#f2f6fc;} QWidget{font-family:'Segoe UI',Arial;font-size:13px;color:#21364f;} QTabWidget::pane{border:0;background:#f2f6fc;} QTabBar::tab{background:#e4edf8;padding:12px 22px;margin-right:3px;color:#23476f;font-weight:600;border-top-left-radius:7px;border-top-right-radius:7px;} QTabBar::tab:selected{background:white;color:#1769d2;border-bottom:3px solid #1769d2;} QPushButton{background:#e8f0fb;border:1px solid #cbdcf1;border-radius:7px;padding:9px 14px;color:#174a85;font-weight:600;} QPushButton:hover{background:#d9e9ff;} QPushButton#primaryButton{background:#1769d2;color:white;border:0;padding:11px 16px;} QPushButton#primaryButton:hover{background:#0f56b4;} QLineEdit,QTextEdit,QComboBox{background:white;border:1px solid #cbd8e8;border-radius:6px;padding:8px;} QGroupBox{font-weight:700;border:1px solid #d5e0ee;border-radius:9px;margin-top:10px;padding-top:12px;background:white;} QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 5px;color:#23476f;} QTableWidget{background:white;border:1px solid #d5e0ee;border-radius:7px;gridline-color:#e6edf5;} QHeaderView::section{background:#eaf1fa;color:#23476f;padding:9px;border:0;font-weight:700;}"); refreshTable();
    }
    QWidget *buildAnalytics() {
        auto *page=new QWidget; auto *root=new QVBoxLayout(page); auto *title=new QLabel("Analytics & Location Tools"); title->setStyleSheet("font-size:20px;font-weight:700;color:#17365e;"); root->addWidget(title);
        auto *summary=new QGroupBox("Report Summary"); auto *sl=new QVBoxLayout(summary); summaryLabel=new QLabel; summaryLabel->setWordWrap(true); sl->addWidget(summaryLabel); root->addWidget(summary);
        auto *mapGroup=new QGroupBox("OpenStreetMap — place search"); auto *ml=new QVBoxLayout(mapGroup); auto *note=new QLabel("Search by a road name, landmark, locality, or full address. The app opens OpenStreetMap in your browser; it does not ask for manual latitude/longitude."); note->setWordWrap(true); auto *row=new QHBoxLayout; auto *place=new QLineEdit; place->setPlaceholderText("e.g. Graphic Era University, Dehradun"); auto *open=new QPushButton("Open Map"); open->setObjectName("primaryButton"); row->addWidget(place,1); row->addWidget(open); ml->addWidget(note); ml->addLayout(row); root->addWidget(mapGroup); root->addStretch();
        connect(open,&QPushButton::clicked,this,[this,place](){openMapForPlace(place->text());});
        return page;
    }
};

int main(int argc,char *argv[]) {
    QApplication app(argc,argv); QApplication::setOrganizationName("SmartRoadProject"); QApplication::setApplicationName("SmartRoad");
    SmartRoadWindow window; window.show(); return app.exec();
}
