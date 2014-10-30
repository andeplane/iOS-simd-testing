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
    vec3() { m_val[0] = 0; m_val[1] = 0; m_val[2] = 0; m_val[3] = 0; }
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

void testSIMD2() {
    int tries = 20;
    int size = 2048;
    vector<vec3> positions(size*size);
    vector<vec3> squares(size*size);
    double totalTime = 0;
    for(int A=0; A<tries; A++) {
        for(int i=0; i<size; i++) {
            for(int j=0; j<size; j++) {
                float x = (i+1)/float(size)*1024;
                float y = (j+1)/float(size)*1024;
                float z = sin(x)*sin(2*y);;
                int idx = index(i,j,size);

                positions[idx] = vec3(x,y,z);
            }
        }

        float *pos = &positions[0][0];
        float *squ = &squares[0][0];
        int N = positions.size()/4;
        QElapsedTimer timer;
        timer.start();
//        N/=4;
        for(int n=0; n<N; n++) {
//            float32x4x4_t vals = vld4q_f32(pos);
//            float32x4x4_t answers;

//            answers.val[0] = vmulq_f32(vals.val[0], vals.val[0]);
//            answers.val[1] = vmulq_f32(vals.val[1], vals.val[1]);
//            answers.val[2] = vmulq_f32(vals.val[2], vals.val[2]);
//            answers.val[3] = vmulq_f32(vals.val[3], vals.val[3]);

//            vst4q_f32(squ, answers);
//            pos += 16;
//            squ += 16;

            float32x4_t val = vld1q_f32(pos);
            float32x4_t ans = vmulq_f32(val, val);

            vst1q_f32(squ, ans);
            pos += 4;
            squ += 4;
        }
        totalTime += timer.elapsed();
        qDebug() << positions[0].x() << " squared is " << squares[0].x();
        qDebug() << positions[1].x() << " squared is " << squares[1].x();
        qDebug() << positions[2].x() << " squared is " << squares[2].x();
        qDebug() << positions[3].x() << " squared is " << squares[3].x() << endl;
    }

    qDebug() << "Finished after " << totalTime/float(1000) << " seconds.";
}

int main(int argc, char *argv[])
{
    testSIMD2();
    return 0;

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
