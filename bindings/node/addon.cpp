#include "api.hpp"
#include <node_api.h>
#include <cstring>

namespace {
using namespace ylp;
struct Pending {};
void ok(napi_env env,napi_status status) {
    if(status==napi_pending_exception)throw Pending{};
    if(status!=napi_ok)throw Error(Code::invalid_metadata,"invalid Node-API argument or allocation");
    bool pending=false;napi_is_exception_pending(env,&pending);if(pending)throw Pending{};
}
std::string string(napi_env env,napi_value value) {
    size_t size=0;ok(env,napi_get_value_string_utf8(env,value,nullptr,0,&size));std::string out(size,'\0');
    ok(env,napi_get_value_string_utf8(env,value,out.data(),size+1,&size));
    if(out.find('\0')!=std::string::npos)throw Error(Code::invalid_metadata,"NUL in binding string");return out;
}
napi_value text(napi_env env,const std::string& s){napi_value out;ok(env,napi_create_string_utf8(env,s.data(),s.size(),&out));return out;}
napi_value json(napi_env env,const Json& j) {
    napi_value out;
    if(j.is_null()){ok(env,napi_get_null(env,&out));return out;}
    if(j.is_boolean()){ok(env,napi_get_boolean(env,j.get<bool>(),&out));return out;}
    if(j.is_string())return text(env,j.get<std::string>());
    if(j.is_number_unsigned()) {
        auto n=j.get<uint64_t>();if(n>9007199254740991ull)ok(env,napi_create_bigint_uint64(env,n,&out));else ok(env,napi_create_double(env,static_cast<double>(n),&out));return out;
    }
    if(j.is_number_integer()) {
        auto n=j.get<int64_t>();if(n>9007199254740991ll||n<-9007199254740991ll)ok(env,napi_create_bigint_int64(env,n,&out));else ok(env,napi_create_double(env,static_cast<double>(n),&out));return out;
    }
    if(j.is_number_float()){ok(env,napi_create_double(env,j.get<double>(),&out));return out;}
    if(j.is_array()) {
        ok(env,napi_create_array_with_length(env,j.size(),&out));for(size_t i=0;i<j.size();++i)ok(env,napi_set_element(env,out,static_cast<uint32_t>(i),json(env,j[i])));return out;
    }
    ok(env,napi_create_object(env,&out));for(auto it=j.begin();it!=j.end();++it)ok(env,napi_set_property(env,out,text(env,it.key()),json(env,it.value())));return out;
}
std::span<const uint8_t> bytes(napi_env env,napi_value value) {
    bool buffer=false;ok(env,napi_is_buffer(env,value,&buffer));
    if(buffer){void* data=nullptr;size_t size=0;ok(env,napi_get_buffer_info(env,value,&data,&size));return {static_cast<const uint8_t*>(data),size};}
    bool typed=false;ok(env,napi_is_typedarray(env,value,&typed));
    if(typed){napi_typedarray_type type;size_t size=0;void* data=nullptr;napi_value array;size_t offset;
        ok(env,napi_get_typedarray_info(env,value,&type,&size,&data,&array,&offset));
        if(type==napi_uint8_array||type==napi_uint8_clamped_array)return {static_cast<const uint8_t*>(data),size};}
    throw Error(Code::invalid_metadata,"Uint8Array or Buffer required");
}
bool function(napi_env env,napi_value value){napi_valuetype type;ok(env,napi_typeof(env,value,&type));return type==napi_function;}
KeyProvider provider(napi_env env,napi_value callback) {
    if(!function(env,callback))return {};
    return [env,callback](const std::string& id,std::array<uint8_t,32>& out){
        napi_value global,result,arg=text(env,id);ok(env,napi_get_global(env,&global));ok(env,napi_call_function(env,global,callback,1,&arg,&result));
        napi_valuetype type;ok(env,napi_typeof(env,result,&type));if(type==napi_null||type==napi_undefined)return false;
        auto b=bytes(env,result);if(b.size()!=32)throw Error(Code::invalid_metadata,"provider must return exactly 32 bytes");std::copy(b.begin(),b.end(),out.begin());return true;
    };
}
void error(napi_env env,const std::exception& e,int code) {
    napi_value message,exception,value;
    if(napi_create_string_utf8(env,e.what(),NAPI_AUTO_LENGTH,&message)!=napi_ok)return;
    napi_create_error(env,nullptr,message,&exception);napi_create_int32(env,code,&value);napi_set_named_property(env,exception,"code",value);napi_throw(env,exception);
}
template<class Fn> napi_value guard(napi_env env,Fn fn){try{return fn();}catch(const Pending&){return nullptr;}catch(const Error& e){error(env,e,static_cast<int>(e.code));}catch(const std::exception& e){error(env,e,static_cast<int>(Code::invalid_metadata));}catch(...){napi_throw_error(env,nullptr,"unexpected native exception");}return nullptr;}
binding::Handle* handle(napi_env env,napi_value value) {void* p=nullptr;ok(env,napi_get_value_external(env,value,&p));if(!p)throw Error(Code::invalid_metadata,"invalid package handle");return static_cast<binding::Handle*>(p);}
napi_value open(napi_env env,napi_callback_info info) {return guard(env,[&]{
    napi_value args[2];size_t n=2;ok(env,napi_get_cb_info(env,info,&n,args,nullptr,nullptr));if(n!=2)throw Error(Code::invalid_metadata,"path and limits JSON required");
    auto h=std::make_unique<binding::Handle>(string(env,args[0]),Json::parse(string(env,args[1])));napi_value out;
    ok(env,napi_create_external(env,h.get(),[](napi_env,void* p,void*){delete static_cast<binding::Handle*>(p);},nullptr,&out));h.release();return out;
});}
napi_value close(napi_env env,napi_callback_info info) {return guard(env,[&]{napi_value arg,out;size_t n=1;ok(env,napi_get_cb_info(env,info,&n,&arg,nullptr,nullptr));if(n!=1)throw Error(Code::invalid_metadata,"package handle required");binding::close(handle(env,arg));ok(env,napi_get_undefined(env,&out));return out;});}
napi_value invoke(napi_env env,napi_callback_info info) {return guard(env,[&]{
    napi_value args[7];size_t n=7;ok(env,napi_get_cb_info(env,info,&n,args,nullptr,nullptr));if(n!=7)throw Error(Code::invalid_metadata,"invalid binding invocation");
    napi_valuetype type;ok(env,napi_typeof(env,args[0],&type));auto h=type==napi_null?nullptr:handle(env,args[0]);
    binding::Callbacks cb;cb.keys=provider(env,args[4]);cb.trust=provider(env,args[5]);
    if(function(env,args[6]))cb.cancel=[env,fn=args[6]]{napi_value global,result;ok(env,napi_get_global(env,&global));ok(env,napi_call_function(env,global,fn,0,nullptr,&result));bool value=false;ok(env,napi_get_value_bool(env,result,&value));return value;};
    auto result=binding::invoke(h,string(env,args[1]),Json::parse(string(env,args[2])),bytes(env,args[3]),cb);napi_value out;
    if(auto p=std::get_if<std::string>(&result))return text(env,*p);
    if(auto p=std::get_if<Json>(&result))return json(env,*p);
    if(auto p=std::get_if<Bytes>(&result)){ok(env,napi_create_buffer_copy(env,p->size(),p->data(),nullptr,&out));return out;}
    ok(env,napi_get_undefined(env,&out));return out;
});}
}
NAPI_MODULE_INIT(){napi_property_descriptor props[]={{"open",nullptr,open,nullptr,nullptr,nullptr,napi_default,nullptr},{"close",nullptr,close,nullptr,nullptr,nullptr,napi_default,nullptr},{"invoke",nullptr,invoke,nullptr,nullptr,nullptr,napi_default,nullptr}};napi_define_properties(env,exports,3,props);return exports;}
