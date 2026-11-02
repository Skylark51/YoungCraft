#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QThread>
#include <iostream>
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);QString mode=qEnvironmentVariable("YOUNGCRAFT_FAKE_XTB_MODE");
 QFile file("xtbopt.log");file.open(QIODevice::WriteOnly);
 file.write("3\n energy: -5.0 gnorm: 0.2\nO 0 0 0\nH 1 0 0\n");file.flush();QThread::msleep(200);
 file.write("H 0 1 0\n");file.flush();QThread::msleep(200);
 file.write("3\n energy: -5.1 gnorm: 0.1\nO 0 0 0\nH 0.95 0 0\nH 0 0.95 0\n");file.flush();
 if(mode=="cancel"){QThread::sleep(20);return 0;}
 QFile final("xtbopt.xyz");final.open(QIODevice::WriteOnly);final.write(mode=="distorted" ? "3\n energy: -5.2 gnorm: 0.01\nO 0 0 0\nH 6 0 0\nH 0 0.96 0\n" : "3\n energy: -5.2 gnorm: 0.01\nO 0 0 0\nH 0.96 0 0\nH 0 0.96 0\n");final.close();
 std::cout<<"| TOTAL ENERGY -5.2 Eh |\n| GRADIENT NORM 0.01 Eh/a0 |\n| HOMO-LUMO GAP 10.0 eV |\n";
 if(mode=="notconverged"){QFile marker("NOT_CONVERGED");marker.open(QIODevice::WriteOnly);std::cout<<"FAILED TO CONVERGE\n";return 1;}
 std::cout<<"GEOMETRY OPTIMIZATION CONVERGED AFTER 2 ITERATIONS\n";return 0;
}
