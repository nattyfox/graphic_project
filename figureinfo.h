#ifndef FIGUREINFO_H
#define FIGUREINFO_H
#include <QColor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QString>
#include <iostream>
#include <windows.h>
#include <QVector>
#include <cmath>
#include <cstdlib>
#include <QDebug>
#include <set>
#include <queue>
#include <qalgorithms.h>
struct Point_temp{
    int Pointx,Pointy;
    Point_temp(int x,int y):Pointx(x),Pointy(y){}
    bool operator <(const Point_temp& b)const{
        return this->Pointx<b.Pointx||(this->Pointx==b.Pointx&&this->Pointy<b.Pointy);
    }
};


enum Shape_serial{fline,fcircle,fellipse,fpolygon,fcurve};
class Shape{
public:
    Shape_serial shape;
    QVector<QPoint> points;
    QVector<QPoint> vertexs;
    QColor color;
    QPoint center;
    int editing;
    bool solid;
    Shape(QColor c):color(c),editing(-1),solid(false){}
    double distance(QPoint Pointx,QPoint Pointy){
        double x=Pointx.x()-Pointy.x();
        double y=Pointx.y()-Pointy.y();
        return sqrt(x*x+y*y);
    }
    QPoint RevolvePoint(double angle,QPoint p,QPoint mid){
        //返回点p绕mid旋转angle后形成的点
        double x=p.x()-mid.x();
        double y=p.y()-mid.y();
        double Deltax=x*cos(angle)-y*sin(angle);
        double Deltay=x*sin(angle)+y*cos(angle);
        return QPoint(mid.x()+Deltax,mid.y()+Deltay);
    }
    QPoint zoomPoint(double ratio,QPoint p,QPoint mid){
        //返回点p绕mid放大ratio倍后的点
        double Deltax=(p.x()-mid.x())*ratio;
        double Deltay=(p.y()-mid.y())*ratio;
        return QPoint(mid.x()+Deltax,mid.y()+Deltay);
    }

    virtual void add(QPoint v){}
    virtual int finish(){}
    virtual bool cut(const QPoint leftdown,int width,int height){}

    virtual void revolve(double angle)=0; //旋转
    virtual void zoom(double ratio)=0; //缩放
    virtual void update()=0;
    virtual void updateMid()=0;
    virtual void fill(){}

    void onVertex(QPoint p){
        for(int i=0;i<vertexs.size();i++)
            if(qAbs(p.x()-vertexs[i].x())<5&&qAbs(p.y()-vertexs[i].y())<5){
                editing=i;break;
            }
    }
    void edit(QPoint p){
        if(editing==-1)return;
        vertexs[editing]=p;
        update();
        updateMid();
    }
    void endEdit(){
        editing=-1;
    }

    virtual void move(QPoint offset)=0;
    void Bresenham(QPoint a,QPoint b){
        int x1=a.x()<b.x()?a.x():b.x();
        int y1=a.x()<b.x()?a.y():b.y();
        int x2=a.x()<b.x()?b.x():a.x();
        int y2=a.x()<b.x()?b.y():a.y();
        int dx=x2-x1;
        int dy=y2-y1;
        int signk=dy>0?1:-1;
        dy=qAbs(dy);
        int p;
        if(dx>dy){
            p=2*dy-dx;
            for(;x1<=x2;x1++){
                points.append(QPoint(x1,y1));
                if(p>=0){
                    y1+=signk;
                    p+=2*(dy-dx);
                }
                else{
                    p+=2*dy;
                }
            }
        }
        else{
            if(y1>y2){
                qSwap(y1,y2);
                qSwap(x1,x2);
            }
            p=2*dx-dy;
            for(;y1<=y2;y1++){
                points.append(QPoint(x1,y1));
                if(p>=0){
                    x1+=signk;
                    p+=2*(dx-dy);
                }
                else p+=2*dx;
            }
        }
    }
};
class line:public Shape{
public:
    line(QPoint p1,QPoint p2,QColor c):Shape(c){
        shape=fline;
        vertexs.push_back(p1);
        vertexs.push_back(p2);
        center=QPoint((p1.x()+p2.x())/2,(p1.y()+p2.y())/2);
        Bresenham(vertexs[0],vertexs[1]);
    }
    void update(){
        points.clear();
        Bresenham(vertexs[0],vertexs[1]);
    }
    void updateMid(){
        QPoint& beg=vertexs[0];
        QPoint& end=vertexs[1];
        center=QPoint((beg.x()+end.x())/2,(beg.y()+end.y())/2);
    }

    bool cut(const QPoint leftdown,int width,int height){
        int minx=leftdown.x(),miny=leftdown.y();
        int maxx=leftdown.x()+width,maxy=leftdown.y()+height;
        int x1 = vertexs[0].x(), y1 = vertexs[0].y();
        int x2 = vertexs[1].x(), y2 = vertexs[1].y();
        int dx = x2-x1, dy = y2-y1;
        int p[4] = {-dx, dx, -dy, dy}; //指示方向：内->外or外->内
        int q[4] = {x1-minx, maxx-x1, y1-miny, maxy-y1}; //指示位置：在内侧or在外侧
        double u1 = 0, u2 = 1; //记录两侧裁剪边界的u值
        for(int i=0;i<4;i++)
        {
            if(p[i]==0) //平行于此边界
            {
                if(q[i]<0) //在此边界的外部，直接丢弃；在内部，则本边界对此直线没有裁剪点，不做更改
                    return false;
            }
            else //与此边界的直线有交点，裁剪
            {
                double r = (double)q[i]/(double)p[i];
                if(p[i]<0) //外->内
                    u1 = u1>r?u1:r;
                else //内->外
                    u2 = u2<r?u2:r;
            }
        }

        if(u1>u2) //裁剪的左侧在右侧的右边，舍弃
            return false;
        vertexs[0].setX(x1+int(u1*dx+0.5));vertexs[0].setY(y1+int(u1*dy+0.5));
        vertexs[1].setX(x1+int(u2*dx+0.5));vertexs[1].setY(y1+int(u2*dy+0.5));
        center=QPoint((vertexs[0].x()+vertexs[1].x())/2,(vertexs[0].y()+vertexs[1].y())/2);
        update();
        return true;
    }

    virtual void move(QPoint offset){
        vertexs[0]+=offset;vertexs[1]+=offset;center+=offset;
        for(int i=0;i<points.size();i++)
            points[i]+=offset;
    }

    virtual void revolve(double angle){ //旋转
        vertexs[0]=RevolvePoint(angle,vertexs[0],center);
        vertexs[1].setX(2*center.x()-vertexs[0].x());
        vertexs[1].setY(2*center.y()-vertexs[0].y());
//        vertex[1]=rotatePoint(angle,vertex[1],center);
        update();
    }
    virtual void zoom(double ratio){ //缩放
        QPoint offset=vertexs[1]-vertexs[0];
        int d=offset.x()*offset.x()+offset.y()*offset.y();
        if(d<1600&&ratio<1)return;
        vertexs[0]=zoomPoint(ratio,vertexs[0],center);
        vertexs[1].setX(2*center.x()-vertexs[0].x());
        vertexs[1].setY(2*center.y()-vertexs[0].y());
//        vertex[1]=zoomPoint(ratio,vertex[1],center);
        update();
    }

};
class curve:public Shape{
public:
    curve(QPoint p,QColor c):Shape(c){
        shape=fcurve;
        vertexs.push_back(p);
    }
    void add(QPoint p){
        vertexs.push_back(p);
        if(vertexs.size()==4){
            producePoints();
            updateMid();
        }
    }

    curve(QPoint p1,QPoint p2,QPoint p3,QPoint p4,QColor c):Shape(c){
        vertexs.push_back(p1);
        vertexs.push_back(p2);
        vertexs.push_back(p3);
        vertexs.push_back(p4);
        //根据vertex计算曲线各点
    }
    virtual void move(QPoint offset){
        center+=offset;
        for(int i=0;i<vertexs.size();i++)
            vertexs[i]+=offset;
        for(int i=0;i<points.size();i++){
            points[i]+=offset;
        }
    }
    void producePoints(){
        for(double u=0;u<=1;u+=0.001)
        {
            double a = pow(1-u,3);
            double b = 3*u*pow(1-u,2);
            double c = 3*pow(u,2)*(1-u);
            double d = pow(u,3);
            points.push_back(QPoint(a*vertexs[0].x() + b*vertexs[1].x() + c*vertexs[2].x() + d*vertexs[3].x() + 0.5,
                                       a*vertexs[0].y() + b*vertexs[1].y() + c*vertexs[2].y() + d*vertexs[3].y() + 0.5));
        }
    }
    void update(){
        points.clear();
        producePoints();
    }
    void updateMid(){
        int minX = vertexs[0].x();
        int maxX = vertexs[0].x();
        int minY = vertexs[0].y();
        int maxY = vertexs[0].y();
        for(QPoint v:vertexs){
            if(v.x()<minX) minX=v.x();
            if(v.x()>maxX) maxX=v.x();
            if(v.y()<minY) minY=v.y();
            if(v.y()>maxY) maxY=v.y();
        }
        center.setX((minX+maxX)/2);center.setY((minY+maxY)/2);
    }

    virtual void revolve(double angle){ //旋转
        for(int i=0;i<vertexs.size();i++)
            vertexs[i]=RevolvePoint(angle,vertexs[i],center);
        update();
    }
    virtual void zoom(double ratio){ //缩放
        int minX = vertexs[0].x();
        int maxX = vertexs[0].x();
        int minY = vertexs[0].y();
        int maxY = vertexs[0].y();
        for(QPoint v:vertexs){
            if(v.x()<minX) minX=v.x();
            if(v.x()>maxX) maxX=v.x();
            if(v.y()<minY) minY=v.y();
            if(v.y()>maxY) maxY=v.y();
        }
        QPoint offset(maxX-minX,maxY-minY);
        int d=offset.x()*offset.x()+offset.y()*offset.y();
        if(d<1600&ratio<1)return;
        for(int i=0;i<vertexs.size();i++)
            vertexs[i]=zoomPoint(ratio,vertexs[i],center);
        update();
    }
};

class circle:public Shape{
public:
    circle(QPoint p1,QPoint p2,QColor c):Shape(c){
        shape=fcircle;
        vertexs.push_back(p1);
        vertexs.push_back(p2);
        center=vertexs[0];
        QPoint& end=vertexs[1];
        producePoints(center,end);
    }
    virtual void move(QPoint offset){
        vertexs[0]+=offset;
        vertexs[1]+=offset;
        center=vertexs[0];
        for(int i=0;i<points.size();i++){
            points[i]+=offset;
        }
    }
    void fill(){
        QPoint offset=vertexs[1]-vertexs[0];
        int r2=offset.x()*offset.x()+offset.y()*offset.y();
        int r=sqrt(r2);
        int miny=vertexs[0].y()-r,maxy=vertexs[0].y()+r;
        int i,j;
        for(int y = miny;y<=maxy;y++){
            int d2=(y-vertexs[0].y())*(y-vertexs[0].y());
            int s2=r2-d2;
            int s=sqrt(s2);
            i=vertexs[0].x()-s;
            j=vertexs[0].x()+s;
            for(int x=i;x<=j;x++)
                points.append(QPoint(x,y));
        }
        solid=true;
    }
    void update(){
        points.clear();
        center=vertexs[0];
        QPoint& end=vertexs[1];
        producePoints(center,end);
        if(solid) fill();
    }
    void updateMid(){
        center=vertexs[0];
    }

    virtual void revolve(double angle){ //旋转
        return;
    }
    virtual void zoom(double ratio){ //缩放
        center=vertexs[0];
        QPoint& end=vertexs[1];
        QPoint offset=end-center;
        int r=offset.x()*offset.x()+offset.y()*offset.y();
        r<<=2;
        if(r<1600&&ratio<1)return;
        end=zoomPoint(ratio,end,center);
        update();
    }
    void producePoints(QPoint a,QPoint b){
        int dx=qAbs(b.x()-a.x()),dy=qAbs(b.y()-a.y());
        int r=sqrt(dx*dx+dy*dy);
        int x=0,y=r;
        int d=1-r;
        int deltax=3;
        int deltay=5-r-r;
        while(x<y)
        {
            if(d<0){
                d+=deltax;
                deltax+=2;
                deltay+=2;
                x++;
            }
            else{
                d+=deltay;
                deltax+=2;
                deltay+=4;
                x++;
                y--;
            }
            //逆时针画点
            points.append(QPoint(a.x()+x,a.y()+y));
            points.append(QPoint(a.x()+y,a.y()+x));
            points.append(QPoint(a.x()+y,a.y()-x));
            points.append(QPoint(a.x()+x,a.y()-y));
            points.append(QPoint(a.x()-x,a.y()-y));
            points.append(QPoint(a.x()-y,a.y()-x));
            points.append(QPoint(a.x()-y,a.y()+x));
            points.append(QPoint(a.x()-x,a.y()+y));
        }
    }
};

class ellipse:public Shape{
public:
    ellipse(QPoint p1,QPoint p2,QColor c):Shape(c){
        shape=fellipse;
        vertexs.push_back(p1);vertexs.push_back(p2);
        QPoint& beg=vertexs[0];
        QPoint& end=vertexs[1];
        beg=p1,end=p2;
        center=QPoint((p1.x()+p2.x())/2,(p1.y()+p2.y())/2);
        producePoints(beg,end);
    }
    virtual void move(QPoint offset){
        QPoint& beg=vertexs[0];
        QPoint& end=vertexs[1];
        beg+=offset;end+=offset;center+=offset;
        for(int i=0;i<points.size();i++)
            points[i]+=offset;
    }
    void fill(){
        QPoint offset=vertexs[1]-vertexs[0];
        //b2*x2+a2*y2=a2*b2=r2
        int a=qAbs(offset.x())>>1;
        int b=qAbs(offset.y())>>1;
        int a2=a*a,b2=b*b;
        int r2=a2*b2;
        int miny=center.y()-b,maxy=center.y()+b;
        for(int y = miny;y<=maxy;y++){
            int y2=(y-center.y())*(y-center.y());
            int x2=(r2-a2*y2)/b2;
            int d=sqrt(x2);
            int i=center.x()-d,j=center.x()+d;
            for(int x=i;x<=j;x++)
                points.append(QPoint(x,y));
        }
        solid=true;
    }

    void update(){
        points.clear();
        QPoint& beg=vertexs[0];
        QPoint& end=vertexs[1];
        producePoints(beg,end);
        if(solid)fill();
    }
    void updateMid(){
        QPoint& beg=vertexs[0];
        QPoint& end=vertexs[1];
        center=QPoint((beg.x()+end.x())/2,(beg.y()+end.y())/2);
    }

    virtual void revolve(double angle){ //旋转
        QPoint& beg=vertexs[0];
        QPoint& end=vertexs[1];
        QPoint offset;
        if(angle>0){
            offset=beg-center;
            beg.setX(center.x()-offset.y());beg.setY(center.y()+offset.x());
            offset=end-center;
            end.setX(center.x()-offset.y());end.setY(center.y()+offset.x());
        }
        else{
            offset=beg-center;
            beg.setX(center.x()+offset.y());beg.setY(center.y()-offset.x());
            offset=end-center;
            end.setX(center.x()+offset.y());end.setY(center.y()-offset.x());
        }
        update();
    }
    virtual void zoom(double ratio){ //缩放
        QPoint& beg=vertexs[0];
        QPoint& end=vertexs[1];
        QPoint offset=end-beg;
        int d=offset.x()*offset.x()+offset.y()*offset.y();
        if(d<1600&&ratio<1)return;
        beg=zoomPoint(ratio,beg,center);
        vertexs[1].setX(2*center.x()-vertexs[0].x());
        vertexs[1].setY(2*center.y()-vertexs[0].y());
//        end=zoomPoint(ratio,end,center);
        update();
    }
    void producePoints(QPoint a,QPoint b){
        int dx=b.x()-a.x(),dy=b.y()-a.y();
        int rx=dx/2,ry=dy/2;
        QPoint mid(a.x()+rx,a.y()+ry);
        rx=qAbs(rx);
        ry=qAbs(ry);
        int rx2=rx*rx,ry2=ry*ry;
        int tworx2=2*rx2,twory2=2*ry2;
        int p=ry2-rx2*ry+rx2/4;
        int x=0,y=ry;
        int px=0,py=tworx2*y;
        while(px<py){
            x++;
            px+=twory2;
            if(p<0)
                p+=ry2+px;
            else{
                y--;
                py-=tworx2;
                p+=ry2+px-py;
            }
            points.append(QPoint(mid.x()+x,mid.y()+y));
            points.append(QPoint(mid.x()+x,mid.y()-y));
            points.append(QPoint(mid.x()-x,mid.y()-y));
            points.append(QPoint(mid.x()-x,mid.y()+y));
        }
        p=ry2*(x+0.5)*(x+0.5)+rx2*(y-1)*(y-1)-rx2*ry2;
        while(y>0){
            y--;
            py-=tworx2;
            if(p>0)
                p+=rx2-py;
            else{
                x++;
                px+=twory2;
                p+=rx2-py+px;
            }
            points.append(QPoint(mid.x()+x,mid.y()+y));
            points.append(QPoint(mid.x()+x,mid.y()-y));
            points.append(QPoint(mid.x()-x,mid.y()-y));
            points.append(QPoint(mid.x()-x,mid.y()+y));
        }
    }
};

class polygon:public Shape{
public:
    polygon(QColor c):Shape(c){
        shape=fpolygon;
    }
    void fill(){
        int miny=points[0].y(),maxy=points[0].y();
        for(QPoint v:vertexs){
            if(v.y()<miny)miny=v.y();
            if(v.y()>maxy)maxy=v.y();
        }
        int sx,sy,tx,ty,x;
        QVector<int> index;
        int size=vertexs.size();
        int *done = new int[size];//用done存放已经做过记录的顶点的下标
        memset(done,0,sizeof(int)*size);
        for(int y = miny;y<=maxy;y++)
        {
            //记录扫描线与边线的交点,扫描每两个顶点(第i个和第j个)连成的边
            for(int i = 0,j = size-1; i < size; j = i, i++) {
                sx = vertexs[i].x();
                sy = vertexs[i].y();
                tx = vertexs[j].x();
                ty = vertexs[j].y();
                int lowy,heighty;
                lowy = (sy<ty)?sy:ty;
                heighty = (sy>ty)?sy:ty;
                //水平线
                if(ty == sy){
                    if(y == ty){
                        int xmax,xmin;
                        xmax = (sx>tx)?sx:tx;
                        xmin = (sx<tx)?sx:tx;
                        for(int xx = xmin;xx<=xmax;xx++){//把水平线上所有点加入points中
                            QPoint p(xx,i);
                            points.append(p);
                        }
                    }
                    continue;
                }
                //没有交点
                if(y<lowy||y>heighty)
                    continue;
                x = sx + (y - sy) * (tx - sx) / (ty - sy);
                //判断交点(x,y)是不是顶点v[i]
                if((x == vertexs[i].x()&&y == vertexs[i].y())){
                    if(done[i]) continue;//第i个顶点已经做了记录
                    //判断顶点是不是极值点
                    //即判断与交点相关联的两条线的另外两个顶点是不是在交点的同一侧
                    int i1=(i+1)%size,i2=(i+size-1)%size;
                    index.push_back(x);
                    //同号、极值点、多记录一次
                    if((vertexs[i1].y()-y)*(vertexs[i2].y()-y)>0){
                        index.push_back(x);
                    }
                    done[i] = 1;//第i个顶点已经做了记录
                    continue;
                }
                //交点(x,y)是不是顶点v[j]
                else if((x == vertexs[j].x()&&y == vertexs[j].y())){
                    if(done[j]) continue;//第j个顶点已经做了记录
                    //判断顶点是不是极值点
                    //即判断与交点相关联的两条线的另外两个顶点是不是在交点的同一侧
                    int j1=(j+1)%size,j2=(j+size-1)%size;
                    index.push_back(x);
                    //同号、极值点、多记录一次
                    if((vertexs[j1].y()-y)*(vertexs[j2].y()-y)>0){
                        index.push_back(x);
                    }
                    done[j] = 1;
                    continue;
                }
                //交点不是顶点
                else index.push_back(x);
            }

            //将index排序
            std::sort(index.begin(),index.end());

            //填充多边形
            for(int n=0,m=n+1;m<index.size();n+=2,m=n+1)
            {
                for(int xx = index[n];xx<=index[m];xx++)
                {
                    QPoint p(xx,y);
                    points.append(p);
                }
            }
            index.clear();
        }
        solid=true;
    }

    void add(QPoint v){
        vertexs.append(v);
        points.append(v);
        int s=vertexs.size();
        if(s>1) Bresenham(vertexs.at(s-2),vertexs.at(s-1));
    }
    void update(){
        points.clear();
        int i=0,j=1;
        for(;j<vertexs.size();j++,i++)
            Bresenham(vertexs.at(i),vertexs.at(j));
        Bresenham(vertexs.at(i),vertexs.at(0));
        if(solid)fill();
    }
    void updateMid(){
        int minx=vertexs.at(0).x(),miny=vertexs.at(0).y();
        int maxx=vertexs.at(0).x(),maxy=vertexs.at(0).y();
        for(int i=1;i<vertexs.size();i++){
            if(vertexs.at(i).x()<minx)
                minx=vertexs.at(i).x();
            if(vertexs.at(i).x()>maxx)
                maxx=vertexs.at(i).x();
            if(vertexs.at(i).y()<miny)
                miny=vertexs.at(i).y();
            if(vertexs.at(i).y()>maxy)
                maxy=vertexs.at(i).y();
        }
        center=QPoint((minx+maxx)/2,(miny+maxy)/2);
    }

    virtual void move(QPoint offset){
        center+=offset;
        for(int i=0;i<vertexs.size();i++)
            vertexs[i]+=offset;
        for(int i=0;i<points.size();i++)
            points[i]+=offset;
    }

    int finish(){
        if(vertexs.size()<3)return 0;
        Bresenham(vertexs.at(vertexs.size()-1),vertexs.at(0));
        updateMid();
        return 1;
    }
    virtual void revolve(double angle){ //旋转
        for(int i=0;i<vertexs.size();i++)
            vertexs[i]=RevolvePoint(angle,vertexs[i],center);
        update();
    }
    virtual void zoom(double ratio){ //缩放
        int minx=vertexs.at(0).x(),miny=vertexs.at(0).y();
        int maxx=vertexs.at(0).x(),maxy=vertexs.at(0).y();
        for(int i=1;i<vertexs.size();i++){
            if(vertexs.at(i).x()<minx)
                minx=vertexs.at(i).x();
            if(vertexs.at(i).x()>maxx)
                maxx=vertexs.at(i).x();
            if(vertexs.at(i).y()<miny)
                miny=vertexs.at(i).y();
            if(vertexs.at(i).y()>maxy)
                maxy=vertexs.at(i).y();
        }
        QPoint offset(maxx-minx,maxy-miny);
        int d=offset.x()*offset.x()+offset.y()*offset.y();
        if(d<1600&&ratio<1)return;
        for(int i=0;i<vertexs.size();i++)
            vertexs[i]=zoomPoint(ratio,vertexs[i],center);
        update();
    }

};

#endif // FIGUREINFO_H
