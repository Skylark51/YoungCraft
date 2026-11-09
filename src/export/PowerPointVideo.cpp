#include "export/PowerPointVideo.h"

#ifdef Q_OS_WIN
#include <QScopeGuard>
#include <memory>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

namespace {
struct ReleaseInterface {
    template<typename T> void operator()(T *value) const { value->Release(); }
};
template<typename T> using Interface = std::unique_ptr<T, ReleaseInterface>;

HRESULT videoType(IMFMediaType **type, const GUID &subtype, QSize size, int fps) {
    HRESULT result=MFCreateMediaType(type);
    if(SUCCEEDED(result))result=(*type)->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);
    if(SUCCEEDED(result))result=(*type)->SetGUID(MF_MT_SUBTYPE,subtype);
    if(SUCCEEDED(result))result=(*type)->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);
    if(SUCCEEDED(result))result=MFSetAttributeSize(*type,MF_MT_FRAME_SIZE,size.width(),size.height());
    if(SUCCEEDED(result))result=MFSetAttributeRatio(*type,MF_MT_FRAME_RATE,fps,1);
    if(SUCCEEDED(result))result=MFSetAttributeRatio(*type,MF_MT_PIXEL_ASPECT_RATIO,1,1);
    if(SUCCEEDED(result))result=(*type)->SetUINT32(MF_MT_YUV_MATRIX,MFVideoTransferMatrix_BT709);
    if(SUCCEEDED(result))result=(*type)->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE,MFNominalRange_16_235);
    return result;
}

void toNv12(const QImage &image, BYTE *buffer) {
    const int width=image.width(),height=image.height();
    BYTE *chroma=buffer+width*height;
    for(int y=0;y<height;y+=2) {
        const auto *top=reinterpret_cast<const QRgb*>(image.constScanLine(y));
        const auto *bottom=reinterpret_cast<const QRgb*>(image.constScanLine(y+1));
        BYTE *row=buffer+y*width;
        for(int x=0;x<width;x+=2) {
            const QRgb pixels[]={top[x],top[x+1],bottom[x],bottom[x+1]};
            int red=0,green=0,blue=0;
            for(int i=0;i<4;++i) {
                const int r=qRed(pixels[i]),g=qGreen(pixels[i]),b=qBlue(pixels[i]);
                row[(i/2)*width+x+i%2]=BYTE(16+(47*r+157*g+16*b+128)/256);
                red+=r;green+=g;blue+=b;
            }
            chroma[x]=BYTE((131584-26*red-86*green+112*blue)/1024);
            chroma[x+1]=BYTE((131584+112*red-102*green-10*blue)/1024);
        }
        chroma+=width;
    }
}

HRESULT writeFrame(IMFSinkWriter *writer,DWORD stream,const QImage &image,int frame,int fps) {
    const DWORD size=DWORD(image.width()*image.height()*3/2);
    IMFMediaBuffer *rawBuffer=nullptr;
    HRESULT result=MFCreateMemoryBuffer(size,&rawBuffer);
    Interface<IMFMediaBuffer> buffer(rawBuffer);
    BYTE *bytes=nullptr;
    if(SUCCEEDED(result))result=buffer->Lock(&bytes,nullptr,nullptr);
    if(SUCCEEDED(result)) {
        toNv12(image,bytes);
        result=buffer->Unlock();
    }
    if(SUCCEEDED(result))result=buffer->SetCurrentLength(size);
    IMFSample *rawSample=nullptr;
    if(SUCCEEDED(result))result=MFCreateSample(&rawSample);
    Interface<IMFSample> sample(rawSample);
    if(SUCCEEDED(result))result=sample->AddBuffer(buffer.get());
    const qint64 timestamp=qint64(frame)*10000000/fps;
    if(SUCCEEDED(result))result=sample->SetSampleTime(timestamp);
    if(SUCCEEDED(result))result=sample->SetSampleDuration(qint64(frame+1)*10000000/fps-timestamp);
    if(SUCCEEDED(result))result=writer->WriteSample(stream,sample.get());
    return result;
}
}
#endif

namespace youngcraft {
bool writePowerPointVideo(const QString &path,QSize size,int fps,int frames,
                          const std::function<QImage(int)> &render,QString &error) {
#ifdef Q_OS_WIN
    auto fail=[&](HRESULT code){
        error=QStringLiteral("Windows H.264 인코딩 실패 (0x%1)").arg(quint32(code),8,16,QChar('0'));
        return false;
    };
    const HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if(FAILED(com)&&com!=RPC_E_CHANGED_MODE)return fail(com);
    const auto releaseCom=qScopeGuard([&]{if(SUCCEEDED(com))CoUninitialize();});
    HRESULT result=MFStartup(MF_VERSION);
    if(FAILED(result))return fail(result);
    const auto shutdown=qScopeGuard([]{MFShutdown();});
    IMFAttributes *rawAttributes=nullptr;
    result=MFCreateAttributes(&rawAttributes,1);
    Interface<IMFAttributes> attributes(rawAttributes);
    if(SUCCEEDED(result))result=attributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING,TRUE);
    IMFSinkWriter *rawWriter=nullptr;
    const auto file=path.toStdWString();
    if(SUCCEEDED(result))result=MFCreateSinkWriterFromURL(file.c_str(),nullptr,attributes.get(),&rawWriter);
    Interface<IMFSinkWriter> writer(rawWriter);
    IMFMediaType *rawOutput=nullptr,*rawInput=nullptr;
    if(SUCCEEDED(result))result=videoType(&rawOutput,MFVideoFormat_H264,size,fps);
    Interface<IMFMediaType> output(rawOutput);
    // Reaction scenes mostly contain stationary backgrounds and linework.
    const int bitrate=qBound(750000,int(qint64(size.width())*size.height()*fps/10),40000000);
    if(SUCCEEDED(result))result=output->SetUINT32(MF_MT_AVG_BITRATE,bitrate);
    DWORD stream=0;
    if(SUCCEEDED(result))result=writer->AddStream(output.get(),&stream);
    if(SUCCEEDED(result))result=videoType(&rawInput,MFVideoFormat_NV12,size,fps);
    Interface<IMFMediaType> input(rawInput);
    if(SUCCEEDED(result))result=writer->SetInputMediaType(stream,input.get(),nullptr);
    if(SUCCEEDED(result))result=writer->BeginWriting();
    if(FAILED(result))return fail(result);
    for(int frame=0;frame<frames;++frame) {
        const auto image=render(frame).convertToFormat(QImage::Format_RGB32);
        if(image.size()!=size){error=QStringLiteral("프레임 렌더링 실패 또는 저장 취소");return false;}
        result=writeFrame(writer.get(),stream,image,frame,fps);
        if(FAILED(result))return fail(result);
    }
    result=writer->Finalize();
    return SUCCEEDED(result)||fail(result);
#else
    Q_UNUSED(path);Q_UNUSED(size);Q_UNUSED(fps);Q_UNUSED(frames);Q_UNUSED(render);
    error=QStringLiteral("PowerPoint 영상 저장은 Windows에서 사용할 수 있습니다.");
    return false;
#endif
}
}
