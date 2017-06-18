#include <QtWidgets>
#include <clipper.hpp>
#include <iostream>

class RenderWidget : public QWidget {
    Q_OBJECT
public:
    RenderWidget(QPlainTextEdit *s_edit, QPlainTextEdit *c_edit, QWidget *parent = nullptr) : QWidget(parent), s_edit_(s_edit), c_edit_(c_edit), zoom_(1.0) {
        startTimer(100);
        clip_type_ = ClipperLib::ClipType::ctXor;
        subj_fill_type_ = clip_fill_type_ = ClipperLib::PolyFillType::pftEvenOdd;
        data_changed_ = true;
        connect(s_edit, &QPlainTextEdit::textChanged, [this] { data_changed_ = true; });
        connect(c_edit, &QPlainTextEdit::textChanged, [this] { data_changed_ = true; });
    }

    void setSettings(ClipperLib::ClipType clip_type, ClipperLib::PolyFillType subj_fill_type, ClipperLib::PolyFillType clip_fill_type) {
        clip_type_ = clip_type;
        clip_fill_type_ = clip_fill_type;
        subj_fill_type_ = subj_fill_type;

        data_changed_ = true;
    }

    void fitInView() {
        QRectF r = solution_path_.boundingRect();
        zoom_ = 1.0 * rect().width() / r.width();
        offset_point_ = -r.topLeft() * zoom_;
    }

protected:
    void timerEvent(QTimerEvent *) {
        if (data_changed_) {
            s_paths_ = parsePaths(s_edit_->toPlainText().toLatin1());
            c_paths_ = parsePaths(c_edit_->toPlainText().toLatin1());

            ClipperLib::Clipper clipper;
            clipper.AddPaths(s_paths_, ClipperLib::PolyType::ptSubject, true);
            clipper.AddPaths(c_paths_, ClipperLib::PolyType::ptClip, true);

            clipper.Execute(clip_type_, solution_paths_, subj_fill_type_, clip_fill_type_);
            data_changed_ = false;
        }

        repaint();
    }

    void mousePressEvent(QMouseEvent *e) { prev_pos_ = e->pos(); }

    void mouseReleaseEvent(QMouseEvent *e) { prev_pos_ = QPoint(); }

    void mouseMoveEvent(QMouseEvent *e) {
        if (!prev_pos_.isNull() && (e->buttons() & Qt::MouseButton::LeftButton)) {
            offset_point_ -= prev_pos_ - e->pos();
            prev_pos_ = e->pos();
            repaint();
        }
    }

    void wheelEvent(QWheelEvent *e) {
        double new_zoom = zoom_;
        if (e->angleDelta().y() > 0) {
            new_zoom *= 1.2;
        } else {
            new_zoom /= 1.2;
        }

        offset_point_ = e->posF() - (new_zoom / zoom_) * (e->posF() - offset_point_);
        zoom_ = new_zoom;
    }
    void paintEvent(QPaintEvent *) {
        QPainter p(this);
        p.setRenderHints(QPainter::RenderHint::NonCosmeticDefaultPen);
        p.fillRect(rect(), Qt::white);

        p.save();

        p.translate(offset_point_);
        p.scale(zoom_, zoom_);

        p.setPen(QPen(QColor(0, 0, 0, 100), 0, Qt::PenStyle::SolidLine, Qt::PenCapStyle::FlatCap, Qt::PenJoinStyle::BevelJoin));

        p.setBrush(QBrush(QColor(9, 33, 114, 70), Qt::BrushStyle::SolidPattern));
        for (auto &path : s_paths_) {
            QPolygon poly;
            for (auto &point : path) {
                poly.append(QPoint(point.X, -point.Y));
            }
            p.drawPolygon(poly);
        }

        p.setBrush(QBrush(QColor(155, 66, 7, 70), Qt::BrushStyle::SolidPattern));
        for (auto &path : c_paths_) {
            QPolygon poly;
            for (auto &point : path) {
                poly.append(QPoint(point.X, -point.Y));
            }
            p.drawPolygon(poly);
        }

        p.setBrush(QBrush(QColor(9, 114, 25, 150), Qt::BrushStyle::SolidPattern));
        solution_path_ = QPainterPath();
        for (auto &path : solution_paths_) {
            QPolygon poly;
            // bool clockwise = true;
            // if (path.size() > 2) {
            //     clockwise = (path[1].X - path[0].X) * (path[1].Y * path[0].Y) < 0;
            // }
            for (auto &point : path) {
                poly.append(QPoint(point.X, -point.Y));
            }

            solution_path_.addPolygon(poly);
        }
        // p.setClipRegion(solution_region);
        // p.fillRect(rect(), p.brush());

        p.drawPath(solution_path_);

        p.restore();

        QRectF mappedRect(-1.0 * QPointF(offset_point_) / zoom_, 1.0 * QSizeF(rect().size()) / zoom_);

        p.drawText(rect(), Qt::AlignTop | Qt::AlignLeft, QString::number(mappedRect.left()) + ", " + QString::number(mappedRect.top()));
        p.drawText(rect(), Qt::AlignBottom | Qt::AlignRight, QString::number(mappedRect.right()) + ", " + QString::number(mappedRect.bottom()));
    }

private:
    ClipperLib::Paths parsePaths(const QByteArray &data) {
        ClipperLib::Paths result;

        QRegularExpression re("\\(\\s*(\\d+\\b[^\\)]*)");
        QRegularExpression re2("(-?[0-9]+(?:\\.[0-9]+)?)[^-0-9]+(-?[0-9]+(?:\\.[0-9]+)?)");
        auto iter = re.globalMatch(data);
        while (iter.hasNext()) {
            auto match = iter.next();
            ClipperLib::Path path;

            auto iter2 = re2.globalMatch(match.capturedRef(1));
            while (iter2.hasNext()) {
                auto match = iter2.next();
                path.emplace_back(match.capturedRef(1).toDouble(), match.capturedRef(2).toDouble());
            }

            if (!path.empty()) {
                result.emplace_back(path);
            }
        }

        return result;
    }

    QPlainTextEdit *s_edit_, *c_edit_;
    bool data_changed_;
    ClipperLib::Paths s_paths_, c_paths_, solution_paths_;
    double zoom_;
    QPointF offset_point_;
    ClipperLib::ClipType clip_type_;
    ClipperLib::PolyFillType subj_fill_type_, clip_fill_type_;
    QPainterPath solution_path_;
    QPoint prev_pos_;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() : QMainWindow() {
        QDockWidget *w1 = new QDockWidget("Subject", this);
        QDockWidget *w2 = new QDockWidget("Clip", this);
        QDockWidget *w3 = new QDockWidget("Settings", this);

        addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, w1);
        addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, w2);
        addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, w3);

        QPlainTextEdit *s_edit = new QPlainTextEdit(w1);
        w1->setWidget(s_edit);

        QPlainTextEdit *c_edit = new QPlainTextEdit(w2);
        w2->setWidget(c_edit);

        QWidget *settings_wgt = new QWidget(w3);
        w3->setWidget(settings_wgt);
        settings_wgt->setLayout(new QHBoxLayout());
        QGroupBox *gbox1 = new QGroupBox("ClipType", settings_wgt);
        gbox1->setLayout(new QVBoxLayout());
        settings_wgt->layout()->addWidget(gbox1);

        QGroupBox *gbox2 = new QGroupBox("subjFillType", settings_wgt);
        gbox2->setLayout(new QVBoxLayout());
        settings_wgt->layout()->addWidget(gbox2);

        QGroupBox *gbox3 = new QGroupBox("clipFillType", settings_wgt);
        gbox3->setLayout(new QVBoxLayout());
        settings_wgt->layout()->addWidget(gbox3);

        QPushButton *fit_in_view_but = new QPushButton("Fit in view", settings_wgt);
        settings_wgt->layout()->addWidget(fit_in_view_but);
        connect(fit_in_view_but, &QPushButton::clicked, [this] { out_wgt_->fitInView(); });

        grp1_ = new QButtonGroup(gbox1);
        grp1_->setExclusive(true);
        grp2_ = new QButtonGroup(gbox2);
        grp2_->setExclusive(true);
        grp3_ = new QButtonGroup(gbox3);
        grp3_->setExclusive(true);

        createRadioButton("ctIntersection", ClipperLib::ctIntersection, grp1_, gbox1);
        createRadioButton("ctUnion", ClipperLib::ctUnion, grp1_, gbox1);
        createRadioButton("ctDifference", ClipperLib::ctDifference, grp1_, gbox1);
        createRadioButton("ctXor", ClipperLib::ctXor, grp1_, gbox1)->setChecked(true);

        createRadioButton("pftEvenOdd", ClipperLib::pftEvenOdd, grp2_, gbox2)->setChecked(true);
        createRadioButton("pftNonZero", ClipperLib::pftNonZero, grp2_, gbox2);
        createRadioButton("pftPositive", ClipperLib::pftPositive, grp2_, gbox2);
        createRadioButton("pftNegative", ClipperLib::pftNegative, grp2_, gbox2);

        createRadioButton("pftEvenOdd", ClipperLib::pftEvenOdd, grp3_, gbox3)->setChecked(true);
        createRadioButton("pftNonZero", ClipperLib::pftNonZero, grp3_, gbox3);
        createRadioButton("pftPositive", ClipperLib::pftPositive, grp3_, gbox3);
        createRadioButton("pftNegative", ClipperLib::pftNegative, grp3_, gbox3);

        // s_edit->setPlainText(
        //     "MULTIPOLYGON(((1943886.2177566 8288039.9482826,1943890.0360151 8288024.6414151,1943898.6410117 8288011.2643791,1943913.9251778 "
        //     "8288009.3565087,1943946.4193372 8287985.4752764,1943969.3400203 8287985.4752764,1943976.0303217 8288010.3214086,1943964.5644142 "
        //     "8288020.8256679,1943917.7545683 8288052.3604649,1943900.555707 8288053.3253704,1943886.2177566 8288039.9482826)))");
        s_edit->setPlainText("(1 1 5 6 4 3)(1 4 4 5 6 2)");
        c_edit->setPlainText("");

        out_wgt_ = new RenderWidget(s_edit, c_edit, this);
        setCentralWidget(out_wgt_);
    }

    void changeOptions() {
        union UserData {
            QObjectUserData *ptr;
            ClipperLib::ClipType clip_type;
            ClipperLib::PolyFillType poly_fill_type;
        } ct, pft1, pft2;

        ct.ptr = grp1_->checkedButton()->userData(1);
        pft1.ptr = grp2_->checkedButton()->userData(1);
        pft2.ptr = grp3_->checkedButton()->userData(1);

        out_wgt_->setSettings(ct.clip_type, pft1.poly_fill_type, pft2.poly_fill_type);
    }

private:
    QRadioButton *createRadioButton(const QString &title, int id, QButtonGroup *group, QWidget *parent) {
        QRadioButton *button = new QRadioButton(title, parent);
        group->setId(button, id);
        group->addButton(button);
        button->setUserData(1, reinterpret_cast<QObjectUserData *>(id));
        parent->layout()->addWidget(button);

        connect(button, &QRadioButton::clicked, this, &MainWindow::changeOptions);
        return button;
    }

    QButtonGroup *grp1_, *grp2_, *grp3_;
    RenderWidget *out_wgt_;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow *wnd = new MainWindow();
    wnd->show();

    return app.exec();
}

#include "qt_main.moc"