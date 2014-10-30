#include <QApplication>
#include <QQmlApplicationEngine>
#include <arm_neon.h>
#include <vector>
#include <cmath>
#include <QDebug>
#include <QVector3D>
#include <QElapsedTimer>
#include <iostream>

using namespace std;

class vec3 {
private:
    float m_val[4];
public:
    vec3(float x, float y, float z) { m_val[0] = x; m_val[1] = y; m_val[2] = z; m_val[3] = 0; }
    vec3(vec3 &vec) { m_val[0] = vec.x(); m_val[1] = vec.y(); m_val[2] = vec.z(); m_val[3] = 0; }

    inline float x() {return m_val[0]; }
    inline float y() {return m_val[1]; }
    inline float z() {return m_val[2]; }

    inline float &operator [](int index) {
        return m_val[index];
    }
};

int index(int i, int j, int size) {
    return i*size + j;
}

int main(int argc, char *argv[])
{
//    QApplication app(argc, argv);

//    QQmlApplicationEngine engine;
//    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    int size = 2048;

    vector<QVector3D> positions(size*size);
    for(int i=0; i<size; i++) {
        for(int j=0; j<size; j++) {
            float x = (i+1)/float(size)*1024;
            float y = (j+1)/float(size)*1024;
            float z = sin(x)*sin(2*y);;
            int idx = index(i,j,size);

            positions[idx] = QVector3D(x,y,z);
        }
    }

    QElapsedTimer timer;
    timer.start();
    for(int i=0; i<positions.size(); i++) {
        positions[i].normalize();
    }
    qDebug() << "Finished after " << timer.restart()/float(1000) << " seconds.";

    for(int i=0; i<size; i++) {
        for(int j=0; j<size; j++) {
            float x = (i+1)/float(size)*1024;
            float y = (j+1)/float(size)*1024;
            float z = sin(x)*sin(2*y);;
            int idx = index(i,j,size);
            positions[idx][0] = x;
            positions[idx][1] = y;
            positions[idx][2] = z;
        }
    }

    vector<QVector3D> squaredNumbers(positions.size());
    vector<float> sumOfSquaredValues(squaredNumbers.size());
    vector<float> oneOverSqrts(sumOfSquaredValues.size());

    cout << "N1: " << 3.0*positions.size()/4.0 << endl;
    cout << "N2: " << oneOverSqrts.size()/4.0 << endl;

    int N = 3*positions.size()/4;
    float *src = &positions[0][0];
    float *dest = &squaredNumbers[0][0];

    timer.start();
    for(int n=0; n<N; n++) {
        float32x4_t v1 = vld1q_f32(src);
        float32x4_t ans = vmulq_f32(v1, v1);

        vst1q_f32(dest, ans);
        src += 4;
        dest += 4;
    }

    for(int n=0; n<squaredNumbers.size()/3; n++) {
        sumOfSquaredValues[n] = squaredNumbers[n][0] + squaredNumbers[n][1] + squaredNumbers[n][2];
    }

    N = oneOverSqrts.size()/4;
    src = &sumOfSquaredValues[0];
    dest = &oneOverSqrts[0];
    for(int n=0; n<N; n++) {
        float32x4_t v1 = vld1q_f32(src);
        float32x4_t ans = vrsqrteq_f32(v1);
        vst1q_f32(dest, ans);
        dest+=4;
        src+=4;
    }

    for(int n=0; n<positions.size(); n++) {
        positions[n][0] *= oneOverSqrts[n];
        positions[n][1] *= oneOverSqrts[n];
        positions[n][2] *= oneOverSqrts[n];
    }

    qDebug() << "SIMD finished after " << timer.restart()/float(1000) << " seconds.";

    qDebug() << "OneOverSqrt: " << oneOverSqrts[0];
    qDebug() << "Length of vectors: ";
    for(int n=0; n<10; n++) {
        qDebug() << positions[n].length();
    }



    return 0;
}
