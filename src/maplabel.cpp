//地图绘制文件
#include<mainwindow.h>
#include<maplabel.h>
#include<astar.h>
#include<QColor>

int global_aa;
int global_bb;

MapLabel::MapLabel(const QString &text, QWidget *parent,int width,int height,int rectaa) : QLabel(text,parent){
    w=width;
    h=height;

    mousestatus=NONE;   //初始化

    square=rectaa;
    border=square/10;   //间隔
    recta=square+border*2;

    this->setGeometry(0,0,w*recta,h*recta);
    this->setFixedSize(w*recta,h*recta);    //固定阵列宽高，以免受到窗口伸缩影响
    this->setText("");

    shutevent=false;    //初始化
    isLPArunning=false;
    isDstarrunning=false;
    isDliterunning=false;
    isLPAMapChanged=false; //地图是否变化
    isDMapChanged=false;
    isDliteMapChanged=false;
    issolved=false;

    mapl=new QLabel(this);
    mapl->setFixedSize(this->size());
    mapl->setAlignment(Qt::AlignCenter); //居中
    bgl=new QLabel(this);
    bgl->setFixedSize(this->size());
    bgl->setAlignment(Qt::AlignCenter); //居中
    bgl->setVisible(false);

    start=new QPoint;
    end=new QPoint;

    init();
}

void MapLabel::mousePressEvent(QMouseEvent *event){ //鼠标点击函数重写
    if(shutevent) return;
    aa=event->pos().y()/recta+1;
    bb=event->pos().x()/recta+1;
    if(event->button()==Qt::LeftButton) { //点击左键 绘制
        if(painterstatus==0){ //绘制障碍物 障碍物允许覆盖起点终点
            switch(status[aa][bb]){
            case 2:
                delete start;
                start=new QPoint;
                break;
            case 3:
                delete end;
                end=new QPoint;
                break;
            }
            status[aa][bb]=1;
            paintnow(aa,bb,1);
            global_aa = aa;
            global_bb = bb;
            mousestatus=LEFT;

            if(isLPArunning){
                isLPAMapChanged=true;
            }else if(isDstarrunning){
                isDMapChanged=true;
            }else if(isDliterunning){
                isDliteMapChanged=true;
            }
        }
        else if(painterstatus==2){ //绘制起点
            if(isLPArunning){
                QMessageBox::information(this,"Caution","LPA*不能变动起点和终点哦~",QMessageBox::Ok);
                return;
            }
            if(isDstarrunning){
                QMessageBox::information(this,"Caution","D*不能变动起点和终点哦~",QMessageBox::Ok);
                return;
            }
            setstartpoint(aa,bb);
        }
        else if(painterstatus==3){ //绘制终点
            if(isLPArunning){
                QMessageBox::information(this,"Caution","LPA*不能变动起点和终点哦~",QMessageBox::Ok);
                return;
            }
            if(isDstarrunning){
                QMessageBox::information(this,"Caution","D*不能变动起点和终点哦~",QMessageBox::Ok);
                return;
            }
            setendpoint(aa,bb);
        }
    }
    if(event->button()==Qt::RightButton) { //点击右键 撤销
        switch(status[aa][bb]){
        case 2:
            if(isLPArunning){
                QMessageBox::information(this,"Caution","LPA*不能变动起点和终点哦~",QMessageBox::Ok);
                return;
            }
            if(isDstarrunning){
                QMessageBox::information(this,"Caution","D*不能变动起点和终点哦~",QMessageBox::Ok);
                return;
            }
            delete start;
            start=new QPoint;
            break;
        case 3:
            if(isLPArunning){
                QMessageBox::information(this,"Caution","LPA*不能变动起点和终点哦~",QMessageBox::Ok);
                return;
            }
            if(isDstarrunning){
                QMessageBox::information(this,"Caution","D*不能变动起点和终点哦~",QMessageBox::Ok);
                return;
            }
            delete end;
            end=new QPoint;
            break;
        }
        status[aa][bb]=0;
        paintnow(aa,bb,0);
        mousestatus=RIGHT;
        if(isLPArunning){
            isLPAMapChanged=true;
        }else if(isDstarrunning){
            isDMapChanged=true;
        }else if(isDliterunning){
            isDliteMapChanged=true;
        }
    }
}
void MapLabel::mouseMoveEvent(QMouseEvent *event){ //鼠标移动函数重写，点击拖动绘制和撤销
    if(shutevent) return;
    aa=event->pos().y()/recta+1;
    bb=event->pos().x()/recta+1;
    if(aa<=0 or aa>h or bb<=0 or bb>w) return;
    if(mousestatus==LEFT and painterstatus==0) {
        if(status[aa][bb]==0){
            status[aa][bb]=1;
            paintnow(aa,bb,1);
        }
    }
    if(mousestatus==RIGHT) {
        if(status[aa][bb]!=0){
            switch(status[aa][bb]){
            case 2:
                delete start;
                start=new QPoint;
                break;
            case 3:
                delete end;
                end=new QPoint;
                break;
            }
            status[aa][bb]=0;
            paintnow(aa,bb,0);
        }
    }
}
void MapLabel::mouseReleaseEvent(QMouseEvent *event){ //鼠标点击释放函数重写
    mousestatus=NONE;
}

//更新地图信息
void MapLabel::setMapPath(){
    //更新前清理上一次地图信息，以免影响更新结果
    nullpath.clear();
    brickspath.clear();
    waypath.clear();

    for(int i=1; i<=h; i++){
        for(int j=1; j<=w; j++){
            if(status[i][j]==0) nullpath.addRoundedRect(border+(j-1)*recta,border+(i-1)*recta,square,square,square/4,square/4); //空白单元
            if(status[i][j]==1) brickspath.addRoundedRect(border+(j-1)*recta,border+(i-1)*recta,square,square,square/4,square/4); //障碍物
            if(status[i][j]==2){ //起点
                start->setX(i);
                start->setY(j);
            }
            if(status[i][j]==3){ //终点
                end->setX(i);
                end->setY(j);
            }
            if(status[i][j]==4) waypath.addRoundedRect(border+(j-1)*recta,border+(i-1)*recta,square,square,square/4,square/4); //最短路径单元
        }
    }
    paintnow(); //开始绘制
}


void MapLabel::init(){ //绘制初始空白地图
    painterstatus=0; //初始化状态
    mappic=QPixmap(this->size());

    nullpath.clear();
    brickspath.clear();
    waypath.clear();

    for(int i=1; i<=h; i++){
        for(int j=1; j<=w; j++){
            status[i][j]=0; //初始都为空白单元
            nullpath.addRoundedRect(border+(j-1)*recta,border+(i-1)*recta,square,square,square/4,square/4);
        }
    }
    paintnow();
}

void MapLabel::paintnow(){

    mappic.fill(Qt::transparent);

    QPainter painter(&mappic); //先绘制在mappic 用于缓存

    painter.setPen(Qt::black);
    painter.setRenderHint(QPainter::Antialiasing); //单元格抗锯齿效果

    painter.setBrush(QBrush(QColor(255,255,255,60))); //需要设置透明度 便于插入背景图
    painter.drawPath(nullpath);
    painter.setBrush(QBrush(QColor("#646485")));//#8686b2 #464150 504b5a
    painter.drawPath(brickspath);
    painter.setBrush(QBrush(QColor("#ffe100")));
    painter.drawPath(waypath);

    painter.end();

    if(!start->isNull()) paintnow(start->x(),start->y(),2);
    if(!end->isNull()) paintnow(end->x(),end->y(),3);

    mapl->setPixmap(mappic);
}
void MapLabel::paintnow(int x,int y,int sta,bool isUpdatenow){
    //qDebug() << "x:" << x << "," << "y:" << y;
    QPainter painter(&mappic);

    painter.setPen(Qt::black);
    painter.setRenderHint(QPainter::Antialiasing);

    switch(sta){
    case 0: //空白
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.eraseRect((y-1)*recta,(x-1)*recta,recta,recta);

        painter.setCompositionMode(QPainter::CompositionMode());
        painter.setBrush(QBrush(QColor(255,255,255,60))); //(255,255,255,60)
        painter.drawRoundedRect(border+(y-1)*recta,border+(x-1)*recta,square,square,square/4,square/4);
        break;
    case 1: //障碍
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.eraseRect((y-1)*recta,(x-1)*recta,recta,recta);

        painter.setBrush(QBrush(QColor("#646485")));//#a4a4d0 #646485
        painter.setCompositionMode(QPainter::CompositionMode());
        painter.drawRoundedRect(border+(y-1)*recta,border+(x-1)*recta,square,square,square/4,square/4);
        break;
    case 2: //起点
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.eraseRect((y-1)*recta,(x-1)*recta,recta,recta);

        painter.setBrush(QBrush(QColor("#bf4040")));
        painter.setCompositionMode(QPainter::CompositionMode());
        painter.drawRoundedRect(border+(y-1)*recta,border+(x-1)*recta,square,square,square/4,square/4);
        break;
    case 3: //终点
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.eraseRect((y-1)*recta,(x-1)*recta,recta,recta);

        painter.setBrush(QBrush(QColor("#46cc66")));
        painter.setCompositionMode(QPainter::CompositionMode());
        painter.drawRoundedRect(border+(y-1)*recta,border+(x-1)*recta,square,square,square/4,square/4);
        break;
    case 4: //路径
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.eraseRect((y-1)*recta,(x-1)*recta,recta,recta);

        painter.setBrush(QBrush(QColor("#ffe100")));
        painter.setCompositionMode(QPainter::CompositionMode());
        painter.drawRoundedRect(border+(y-1)*recta,border+(x-1)*recta,square,square,square/4,square/4);
        break;
    }

    painter.end();
    if(isUpdatenow) mapl->setPixmap(mappic); //更新地图

}
void MapLabel::setstartpoint(int x, int y){ //设置起点
    if(!start->isNull() and status[start->x()][start->y()]==2){
        paintnow(start->x(),start->y(),0);
        status[start->x()][start->y()]=0;
    }
    start->setX(x);
    start->setY(y);
    status[x][y]=2;
    paintnow(x,y,2);
}
void MapLabel::setendpoint(int x, int y){ //设置终点
    if(!end->isNull() and status[end->x()][end->y()]==3){
        paintnow(end->x(),end->y(),0);
        status[end->x()][end->y()]=0;
    }
    end->setX(x);
    end->setY(y);
    status[x][y]=3;
    paintnow(x,y,3);
}
void MapLabel::setpainterstatus(int sta){
    painterstatus=sta;
}
void MapLabel::setbackground(QImage &bg){ //设置背景图片
    backpic=bg;

    bgl->clear();
    bgl->setFixedSize(this->size());
    bgl->setPixmap(QPixmap::fromImage(QImage(bg.scaled(this->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation))));
    bgl->stackUnder(mapl);
    bgl->setVisible(true);
    //bgl->show();
}
void MapLabel::clearbackground(){ //清除背景图片 开关式
    bgl->setVisible(false);
}
void MapLabel::setRect(int a){ //单元格样式
    square=a; //边长
    border=square/10; //圆角
    recta=square+border*2; //间隔

    this->setGeometry(0,0,w*recta,h*recta);
    this->setFixedSize(w*recta,h*recta);
    this->setText("");
    mapl->setFixedSize(this->size());
    bgl->setFixedSize(this->size());

    mappic=QPixmap(this->size());

    if(bgl->isVisible()){
        bgl->setPixmap(QPixmap::fromImage(backpic.scaled(this->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation)));
    }
    setMapPath();
}

int MapLabel::rsizeindex(){
    switch (square) {
    case 20:return 0;
    case 30:return 1;
    case 45:return 2;
    case 60:return 3;
    }
}
