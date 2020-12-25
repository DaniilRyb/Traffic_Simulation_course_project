#include <QRegularExpression>
#include <QTimer>
#include <QPainter>
#include <QKeyEvent>
#include <cmath>
#include <random>
#include <Bus.h>
#include "Car.h"
#include "CarFactory.h"
#include "TrafficLight.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

bool MainWindow::debugMode = false;

void MainWindow::initGraph()
{
    auto children = this->findChildren<QLabel *>(QRegularExpression{"path_help"});
    for (const auto &child : children) {
        // считываем свойства

        int vId = child->property("vId").toInt();
        int gId = child->property("gId").toInt();
        qreal velocityMax = child->property("velocityMax").toDouble();
        qreal velocity = child->property("velocity").isNull()
                         ? 0 : child->property("velocity").toDouble();
        int lightId = child->property("lightId").toInt();
        int lightGroup = child->property("lightGroup").toInt();
        int spawnRadius = child->property("spawnRadius").toInt();
        bool isBusStop = child->property("busStop").toBool();

        if (!trafficPaths[gId]) {
            trafficPaths[gId] = new TrafficPath{};
        }
        QRect rect = child->geometry();

        auto point = new TrafficPathPoint{
            static_cast<qreal>(rect.x() + rect.width() / 2),
            static_cast<qreal>(rect.y() + rect.height() / 2),
        };
        point->setId(vId, gId);
        point->setVelocityMax(velocityMax);
        point->setVelocity(velocity);
        point->setSpawnRadius(spawnRadius);
        point->setIsBusStop(isBusStop);

        if (lightId != -1) {
            point->setLight(trafficLights[lightGroup]->getLights()[lightId]);
        }

        if (trafficPaths[gId]->getPoints()[vId]) {
            qWarning() << gId << vId << "point already created";
        }
        trafficPaths[gId]->addPoint(point, vId);
    }

    // А теперь инициализируем боковые вершины
    for (const auto &child : children) {
        // считываем свойства

        int vId = child->property("vId").toInt();
        int gId = child->property("gId").toInt();

        int leftVId = child->property("leftVId").isNull() ? -1 : child->property("leftVId").toInt();
        int leftGId = child->property("leftGId").isNull() ? -1 : child->property("leftGId").toInt();
        int rightVId = child->property("rightVId").isNull() ? -1 : child->property("rightVId").toInt();
        int rightGId = child->property("rightGId").isNull() ? -1 : child->property("rightGId").toInt();

        auto &point = trafficPaths[gId]->getPoints()[vId];
        point->setSidePoints(
            leftVId == -1 ? nullptr : trafficPaths[leftGId]->getPoints()[leftVId],
            rightVId == -1 ? nullptr : trafficPaths[rightGId]->getPoints()[rightVId]
        );
    }

}

void MainWindow::initLights()
{
    auto children = this->findChildren<QLabel *>(QRegularExpression{"light_help"});
    for (const auto &child : children) {

        int id = child->property("id").toInt();
        bool isPed = child->property("isPed").toBool();
        int group = child->property("group").toInt();
        qreal rotation = child->property("rotation").toDouble();

        auto *light = new TrafficLight{
            static_cast<int>(rotation),
            isPed,
            this
        };


        light->setGeometry(child->geometry());
        light->updateImage();

        if (!trafficLights[group]) {
            trafficLights[group] = new TrafficLightSystem{};

            updateDataList.push_back(trafficLights[group]);
        }
        if (!isPed) {
            trafficLights[group]->addLight(light, id);
        } else {
            trafficLights[group]->addPedLight(light);
        }

        updateImageList.push_back(light);
    }

    trafficLights[0]->setResetTime(10000, 10000, 10000);
    trafficLights[1]->setResetTime(5000, 10000, 6000);
    trafficLights[2]->setResetTime(5000, 10000, 7000);
    trafficLights[3]->setResetTime(5000, 10000, 5000);
    trafficLights[4]->setResetTime(5000, 6000, 4000);
}

void MainWindow::initPeds()
{
    auto children = this->findChildren<QLabel *>(QRegularExpression{"ped_help"});
    for (const auto &child : children) {

        bool isHorizontal = child->property("isHorizontal").toBool();
        int group = child->property("group").toInt();

        auto *pedSystem = new PedSystem{
            isHorizontal,
            child->geometry(),
            trafficLights[group],
            this
        };
        pedSystems.push_back(pedSystem);

        updateImageList.push_back(pedSystem);
        updateDataList.push_back(pedSystem);
    }
}

void MainWindow::initBackground()
{
    background = new QLabel{this};
    background->setGeometry(0, 0, 800, 600);

    background->setPixmap(QPixmap{":/image/Background.png"});
    background->show();
    background->lower();

    QSize size{background->rect().width(), background->rect().height()};
    this->setMaximumSize(size);
    this->setMinimumSize(size);
}

MainWindow::MainWindow()
    : ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initBackground();
    initLights();
    initGraph();
    initPeds();

    auto *logic = new QTimer{this};
    int interval = 1000 / 60;
    logic->setInterval(interval);

    connect(logic, &QTimer::timeout, [this, interval]() {
        for (auto *updatable: updateDataList) {
            updatable->updateData(interval);
        }
    });
    logic->start();

    auto *graphics = new QTimer{this};
    graphics->setInterval(1000 / 40);
    connect(graphics, &QTimer::timeout, [this]() {
        for (auto *updatable: updateImageList) {
            updatable->updateImage();
        }
    });
    graphics->start();

    // отсюда взята перестройка машин

    // Added new cars every 3.5 seconds
    auto *carTimer = new QTimer{this};
    //auto *carTimer1 = new QTimer{this};
   // auto *carTimer2 = new QTimer{this};


    carTimer->setInterval(3500);


    connect(carTimer, &QTimer::timeout, [this]() {
        TrafficPath *path = nullptr;
        TrafficPath *path1= nullptr;
        TrafficPath *path2= nullptr;
        TrafficPath *path3= nullptr;


        size_t attempts = 1;
        while (path == nullptr && attempts < 10) {
            std::mt19937 generator{std::random_device{}()};
            std::mt19937 generator1{std::random_device{}()};
            std::mt19937 generator2{std::random_device{}()};
            std::mt19937 generator3{std::random_device{}()};


            std::uniform_int_distribution<size_t> random(0, trafficPaths.size() - 1);
            std::uniform_int_distribution<size_t> random1(1, trafficPaths.size() - 1);
            std::uniform_int_distribution<size_t> random2(4, trafficPaths.size() - 1);
            std::uniform_int_distribution<size_t> random3(7, trafficPaths.size() - 1);

            path = trafficPaths[random(generator)];
            path1 = trafficPaths[random1(generator1)];
            path2 = trafficPaths[random2(generator2)];
            path3 = trafficPaths[random3(generator3)];


            auto point = path->getPoints()[0];

            Car::Circle collision{
                *dynamic_cast<QPointF *>(point),
                static_cast<qreal>(point->getSpawnRadius()),

            };
            if (isCarsInCollision({collision}, nullptr) != -1) {
                path = nullptr;
                ++attempts;
            }

    }


        if (!path) {
            return;
        }


       /* auto *turnTimer = new QTimer{this};
        turnTimer->setInterval(5000);                       // попытка перестроения каждые 5 секунд
        connect(turnTimer, &QTimer::timeout, [this]() {
            std::vector<Car *> shuffleCars(cars.cbegin(), cars.cend());
           // std::vector<Bus *> shuffleBus(bus.cbegin(), bus.cend());

            std::shuffle(
                shuffleCars.begin(),shuffleCars.end(),std::mt19937{std::random_device{}()});

            for (Car *car : shuffleCars) {
                for (bool left : {true, false}) {
                    if (car->doTurn(left)) {
                        return;
                    }
                }
            }
        });
       // turnTimer->start();
*/
        Car *car = nullptr;
        Car *car1 = nullptr;
        Car *car2 = nullptr;
        Car *car3 = nullptr;

        car = CarFactory::createCar(this, path);
        car1 = CarFactory::createCar(this, path1);
        car2 = CarFactory::createCar(this, path2);
        car3 = CarFactory::createCar(this, path3);


        car->show();
        car1->show();
        car2->show();
        car3->show();
        cars.push_back(car);
        cars.push_back(car1);
        cars.push_back(car2);
        cars.push_back(car3);

        updateImageList.push_back(car);
        updateDataList.push_back(car);
        updateImageList.push_back(car1);
        updateDataList.push_back(car1);
        updateImageList.push_back(car2);
        updateDataList.push_back(car2);
        updateImageList.push_back(car3);
        updateDataList.push_back(car3);
    });
    carTimer->start();
}

int MainWindow::isCarsInCollision(const QVector<Car::Circle> &circles, Car *except)
{
    for (int i = 0; i < circles.size(); ++i) {
        for (Car *car: cars) {
            if (car == except) {
                continue;
            }

            auto pivots = car->getPivots();
            for (const auto &pivot :pivots) {
                QPointF delta = pivot - circles[i].point;

                qreal length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
                if (length <= circles[i].radius) {
                    return i;
                }
            }
        }
    }

    return -1;
}

void MainWindow::removeCar(Car *car)
{
    cars.removeOne(car);
    updateDataList.removeOne(car);
    updateImageList.removeOne(car);

    car->deleteLater();
}

const QVector<TrafficPath *> &MainWindow::getTrafficPaths() const
{
    return trafficPaths;
}

void MainWindow::updateImage()
{
    auto children = this->findChildren<QLabel *>(QRegularExpression{"help"});
    for (const auto &child : children) {
        child->setVisible(debugMode);
    }
}
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);

    if (event->key() == Qt::Key_F10) {
        debugMode = !debugMode;
    }
}

MainWindow::~MainWindow()
{
    for (auto *car : cars) {
        delete car;
    }
    for (auto *path: trafficPaths) {
        delete path;
    }
    for (auto *light: trafficLights) {
        delete light;
    }
    for (auto *peds: pedSystems) {
        delete peds;
    }

    delete background;
    delete ui;
}

QLabel *MainWindow::drawCircle(const Car::Circle &circle, const QColor &color)
{
    auto label = new QLabel{this};
    label->setGeometry(
        circle.point.x() - circle.radius,
        circle.point.y() - circle.radius,
        circle.radius * 2,
        circle.radius * 2
    );

    label->show();

    QPixmap newBackgroundPixmap(circle.radius * 2, circle.radius * 2);
    newBackgroundPixmap.fill(Qt::transparent);

    QPainter painter{&newBackgroundPixmap};
    painter.setPen(color);
    painter.drawEllipse(
        0,
        0,
        circle.radius * 2 - 2,
        circle.radius * 2 - 2
    );
    painter.end();

    label->setPixmap(newBackgroundPixmap);
    return label;
}

QLabel *MainWindow::drawSquare(const Car::Circle &circle, const QColor &color)
{
    auto label = new QLabel{this};
    label->setGeometry(
        circle.point.x() - circle.radius,
        circle.point.y() - circle.radius,
        circle.radius * 2,
        circle.radius * 2
    );
    QString colorString = QString::number(color.red()) + ", "
        + QString::number(color.green()) + ", "
        + QString::number(color.blue()) + ", "
        + QString::number(color.alpha());
    label->setStyleSheet("border: 1px solid rgba(" + colorString + ");");

    label->show();
    return label;
}
