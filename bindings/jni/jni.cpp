#include "api.hpp"
#include <jni.h>
#include <climits>

namespace {
using namespace ylp;
struct Pending {};
void pending(JNIEnv* env){if(env->ExceptionCheck())throw Pending{};}
jstring text(JNIEnv* env,const std::string& value) {
    if(env->PushLocalFrame(12)!=0)throw Pending{};
    try {
        if(value.size()>INT_MAX)throw Error(Code::resource_limit,"Java string limit");
        auto bytes=env->NewByteArray(static_cast<jsize>(value.size()));pending(env);
        env->SetByteArrayRegion(bytes,0,static_cast<jsize>(value.size()),reinterpret_cast<const jbyte*>(value.data()));pending(env);
        auto cls=env->FindClass("java/lang/String");pending(env);
        auto ctor=env->GetMethodID(cls,"<init>","([BLjava/lang/String;)V");pending(env);
        auto charset=env->NewStringUTF("UTF-8");pending(env);
        auto result=env->NewObject(cls,ctor,bytes,charset);pending(env);
        return static_cast<jstring>(env->PopLocalFrame(result));
    } catch(...){env->PopLocalFrame(nullptr);throw;}
}
std::string string(JNIEnv* env,jstring value) {
    if(!value)throw Error(Code::invalid_metadata,"null Java string");
    auto count=env->GetStringLength(value);const jchar* chars=env->GetStringChars(value,nullptr);pending(env);
    if(!chars)throw Pending{};std::string out;
    try {
        for(jsize i=0;i<count;++i){uint32_t cp=chars[i];
            if(cp==0)throw Error(Code::invalid_metadata,"NUL in binding string");
            if(cp>=0xd800&&cp<=0xdbff){if(++i>=count||chars[i]<0xdc00||chars[i]>0xdfff)throw Error(Code::invalid_metadata,"invalid UTF-16 surrogate");cp=0x10000+((cp-0xd800)<<10)+(chars[i]-0xdc00);}
            else if(cp>=0xdc00&&cp<=0xdfff)throw Error(Code::invalid_metadata,"unpaired UTF-16 surrogate");
            if(cp<0x80)out.push_back(static_cast<char>(cp));
            else if(cp<0x800){out.push_back(static_cast<char>(0xc0|(cp>>6)));out.push_back(static_cast<char>(0x80|(cp&63)));}
            else if(cp<0x10000){out.push_back(static_cast<char>(0xe0|(cp>>12)));out.push_back(static_cast<char>(0x80|((cp>>6)&63)));out.push_back(static_cast<char>(0x80|(cp&63)));}
            else{out.push_back(static_cast<char>(0xf0|(cp>>18)));out.push_back(static_cast<char>(0x80|((cp>>12)&63)));out.push_back(static_cast<char>(0x80|((cp>>6)&63)));out.push_back(static_cast<char>(0x80|(cp&63)));}
        }
    }catch(...){env->ReleaseStringChars(value,chars);throw;}
    env->ReleaseStringChars(value,chars);return out;
}
jobject json(JNIEnv* env,const Json& value) {
    if(env->PushLocalFrame(24)!=0)throw Pending{};
    try {
        jobject out=nullptr;
        if(value.is_string())out=text(env,value.get<std::string>());
        else if(value.is_boolean()){auto cls=env->FindClass("java/lang/Boolean");pending(env);auto method=env->GetStaticMethodID(cls,"valueOf","(Z)Ljava/lang/Boolean;");pending(env);out=env->CallStaticObjectMethod(cls,method,static_cast<jboolean>(value.get<bool>()));}
        else if(value.is_number_unsigned()&&value.get<uint64_t>()>INT64_MAX){auto cls=env->FindClass("java/math/BigInteger");pending(env);auto method=env->GetMethodID(cls,"<init>","(Ljava/lang/String;)V");pending(env);out=env->NewObject(cls,method,text(env,std::to_string(value.get<uint64_t>())));}
        else if(value.is_number_integer()){auto cls=env->FindClass("java/lang/Long");pending(env);auto method=env->GetStaticMethodID(cls,"valueOf","(J)Ljava/lang/Long;");pending(env);out=env->CallStaticObjectMethod(cls,method,static_cast<jlong>(value.get<int64_t>()));}
        else if(value.is_number_float()){auto cls=env->FindClass("java/lang/Double");pending(env);auto method=env->GetStaticMethodID(cls,"valueOf","(D)Ljava/lang/Double;");pending(env);out=env->CallStaticObjectMethod(cls,method,value.get<double>());}
        else if(value.is_array()){
            auto cls=env->FindClass("java/util/ArrayList");pending(env);auto ctor=env->GetMethodID(cls,"<init>","(I)V");auto add=env->GetMethodID(cls,"add","(Ljava/lang/Object;)Z");pending(env);
            out=env->NewObject(cls,ctor,static_cast<jint>(value.size()));pending(env);
            for(const auto& item:value){auto child=json(env,item);env->CallBooleanMethod(out,add,child);pending(env);if(child)env->DeleteLocalRef(child);}
        }else if(value.is_object()){
            auto cls=env->FindClass("java/util/LinkedHashMap");pending(env);auto ctor=env->GetMethodID(cls,"<init>","()V");auto put=env->GetMethodID(cls,"put","(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");pending(env);
            out=env->NewObject(cls,ctor);pending(env);
            for(auto it=value.begin();it!=value.end();++it){auto key=text(env,it.key());auto child=json(env,it.value());auto old=env->CallObjectMethod(out,put,key,child);pending(env);env->DeleteLocalRef(key);if(child)env->DeleteLocalRef(child);if(old)env->DeleteLocalRef(old);}
        }
        pending(env);return env->PopLocalFrame(out);
    }catch(...){env->PopLocalFrame(nullptr);throw;}
}
KeyProvider provider(JNIEnv* env,jobject callback){
    if(!callback)return {};
    return [env,callback](const std::string& id,std::array<uint8_t,32>& out){
        if(env->PushLocalFrame(12)!=0)throw Pending{};
        try{auto cls=env->GetObjectClass(callback);auto method=env->GetMethodID(cls,"keyFor","(Ljava/lang/String;)[B");pending(env);
            auto result=static_cast<jbyteArray>(env->CallObjectMethod(callback,method,text(env,id)));pending(env);
            if(!result){env->PopLocalFrame(nullptr);return false;}
            if(env->GetArrayLength(result)!=32)throw Error(Code::invalid_metadata,"key provider must return exactly 32 bytes");
            env->GetByteArrayRegion(result,0,32,reinterpret_cast<jbyte*>(out.data()));pending(env);env->PopLocalFrame(nullptr);return true;
        }catch(...){env->PopLocalFrame(nullptr);throw;}
    };
}
void error(JNIEnv* env,const std::exception& e,int code){
    if(env->ExceptionCheck())return;
    auto cls=env->FindClass("xyz/younglion/ylplugin/YLPluginException");if(!cls)return;
    auto ctor=env->GetMethodID(cls,"<init>","(Ljava/lang/String;I)V");if(!ctor)return;
    try{auto object=static_cast<jthrowable>(env->NewObject(cls,ctor,text(env,e.what()),code));if(object)env->Throw(object);}catch(...){}
}
template<class Result,class Fn>Result guard(JNIEnv* env,Result fallback,Fn fn){try{return fn();}catch(const Pending&){}catch(const Error& e){error(env,e,static_cast<int>(e.code));}catch(const std::exception& e){error(env,e,static_cast<int>(Code::invalid_metadata));}catch(...){auto cls=env->FindClass("java/lang/RuntimeException");if(cls)env->ThrowNew(cls,"unexpected native exception");}return fallback;}
binding::Handle* handle(jlong raw){return reinterpret_cast<binding::Handle*>(static_cast<intptr_t>(raw));}
}
extern "C" JNIEXPORT jlong JNICALL Java_xyz_younglion_ylplugin_Native_open(JNIEnv* env,jclass,jstring path,jstring limits){return guard<jlong>(env,0,[&]{auto h=std::make_unique<binding::Handle>(string(env,path),Json::parse(string(env,limits)));return static_cast<jlong>(reinterpret_cast<intptr_t>(h.release()));});}
extern "C" JNIEXPORT void JNICALL Java_xyz_younglion_ylplugin_Native_close(JNIEnv* env,jclass,jlong raw){guard<int>(env,0,[&]{auto h=handle(raw);binding::close(h);delete h;return 1;});}
extern "C" JNIEXPORT jobject JNICALL Java_xyz_younglion_ylplugin_Native_invoke(JNIEnv* env,jclass,jlong raw,jstring operation,jstring request,jbyteArray input,jobject keys,jobject trust,jobject cancellation){return guard<jobject>(env,nullptr,[&]()->jobject{
    struct Secret {Bytes bytes;~Secret(){volatile uint8_t* p=bytes.data();for(size_t i=0;i<bytes.size();++i)p[i]=0;}} binary;
    if(input){auto size=env->GetArrayLength(input);binary.bytes.resize(static_cast<size_t>(size));env->GetByteArrayRegion(input,0,size,reinterpret_cast<jbyte*>(binary.bytes.data()));pending(env);}
    binding::Callbacks cb;cb.keys=provider(env,keys);cb.trust=provider(env,trust);
    if(cancellation)cb.cancel=[env,cancellation]{auto cls=env->GetObjectClass(cancellation);auto method=env->GetMethodID(cls,"cancelled","()Z");pending(env);auto result=env->CallBooleanMethod(cancellation,method);pending(env);env->DeleteLocalRef(cls);return result!=JNI_FALSE;};
    auto result=binding::invoke(handle(raw),string(env,operation),Json::parse(string(env,request)),binary.bytes,cb);
    if(auto p=std::get_if<std::string>(&result))return text(env,*p);
    if(auto p=std::get_if<Json>(&result))return json(env,*p);
    if(auto p=std::get_if<Bytes>(&result)){if(p->size()>INT_MAX)throw Error(Code::resource_limit,"Java byte array limit");auto out=env->NewByteArray(static_cast<jsize>(p->size()));pending(env);env->SetByteArrayRegion(out,0,static_cast<jsize>(p->size()),reinterpret_cast<const jbyte*>(p->data()));pending(env);return out;}
    return nullptr;
});}
