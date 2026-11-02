#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>
#include <string>
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv); std::string line;std::getline(std::cin,line);
 auto input=QJsonDocument::fromJson(QByteArray::fromStdString(line)).object();
 int maximum=input["max_iterations"].toInt(),step=0;
 QJsonObject out{{"type","ready"},{"field","UFF"},{"iteration",0},{"energy",100.0},{"rms_gradient",1.0},{"max_gradient",2.0}};
 auto send=[&]{std::cout<<QJsonDocument(out).toJson(QJsonDocument::Compact).constData()<<std::endl;};send();
 while(std::getline(std::cin,line)) {
  if(line!="chunk")return 2;
  step=qMin(maximum,step==0?8:step==8?48:step+8);
  QJsonArray positions;for(auto a:input["atoms"].toArray()){auto p=a.toObject()["p"].toArray();p[0]=p[0].toDouble()+step*0.001;positions.append(p);}
  bool converged=qEnvironmentVariableIntValue("YOUNGCRAFT_FAKE_CONVERGE")>0&&step>=64;
  out["type"]=(step>=maximum||converged)?"done":"step";out["iteration"]=step;out["energy"]=100.0-step;out["delta_energy"]=-8.0;out["positions"]=positions;out["converged"]=converged;send();
  if(step>=maximum||converged)return 0;
 }
}
