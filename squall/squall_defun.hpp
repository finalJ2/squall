#ifndef SQUALL_DEFUN_HPP_
#define SQUALL_DEFUN_HPP_

#include <squirrel.h>
#include "squall_stack_operation.hpp"
#include "squall_partial_apply.hpp"

//#include "squall_demangle.hpp"
#include <iostream>

namespace squall {

namespace detail {

////////////////////////////////////////////////////////////////
// defun
template <class R> inline
int stub_aux(
    HSQUIRRELVM vm, const std::function<R ()>& f, SQInteger index) {

    push<R>(vm, f());
    return 1;
}

template <> inline
int stub_aux<void>(
    HSQUIRRELVM vm, const std::function<void ()>& f, SQInteger index) {

    f();
    return 0;
}

template <class R, class H, class... T> inline
int stub_aux(
    HSQUIRRELVM vm, const std::function<R (H, T...)>& f, SQInteger index) {

    std::function<R (T...)> newf = partial(f, fetch<H>(vm, index));
    return stub_aux(vm, newf, index + 1);
}

/*
inline
void dp(HSQUIRRELVM vm, int index) {
    SQInteger arg;
    int succeed = SQ_SUCCEEDED(sq_getinteger(vm, index, &arg));
    SQObjectType t = sq_gettype(vm, index);
    printf("stub%d: %lld, succeed = %d, type = %08x\n", index, arg, succeed, t);
}
*/

template <int Offset, class F>
SQInteger stub(HSQUIRRELVM vm) {
    try {
        SQUserPointer fp;
        if(!SQ_SUCCEEDED(sq_getuserdata(vm, -1, &fp, NULL))) { assert(0); }
        const F& f = **((F**)fp);

        //return stub_aux(klass_table, f, Offset);
        SQInteger x = stub_aux(vm, f, Offset);
        return x;
    }
    catch(std::exception& e) {
        return sq_throwerror(
            vm, (std::string("error in callback: ") + e.what()).c_str());
    }
}

template <class T> char typemask() { return '.'; }

template <> char typemask<decltype(nullptr)>() { return 'o'; }
template <> char typemask<int>() { return 'i'; }
template <> char typemask<float>() { return 'f'; }
template <> char typemask<bool>() { return 'b'; }
template <> char typemask<const char*>() { return 's'; }
template <> char typemask<std::string>() { return 's'; }

template <class... T>
struct TypeMaskList;

template <>
struct TypeMaskList<> {
    static std::string doit() { return ""; }
};

template <class H, class... T>
struct TypeMaskList<H, T...> {
    static std::string doit() {
        return typemask<H>() + TypeMaskList<T...>::doit();
    }
};

template <int Offset, class R, class... T>
void defun(
    HSQUIRRELVM vm,
    const std::string& name,
    const std::function<R (T...)>& f,
    const std::string& argtypemask) {

    sq_pushstring(vm, name.data(), name.length());
    construct_object(vm, f);
    sq_newclosure(vm, stub<Offset, std::function<R (T...)>>, 1);
    sq_setparamscheck(vm, SQ_MATCHTYPEMASKSTRING, argtypemask.c_str());
    sq_setnativeclosurename(vm, -1, name.c_str());
    sq_newslot(vm, -3, SQFalse);
}

template <class R, class... T>
void defun_global(
    HSQUIRRELVM vm, const HSQOBJECT& table,
    const std::string& name, const std::function<R (T...)>& f) {

    //sq_pushroottable(vm);
    sq_pushobject(vm, table);
    defun<2>(vm, name, f, "." + TypeMaskList<T...>::doit());
    sq_pop(vm, 1);
}

template <class R, class... T>
void defun_local(
    HSQUIRRELVM vm, const HSQOBJECT& klass_object,
    const std::string& name, const std::function<R (T...)>& f) {

    sq_pushobject(vm, klass_object);
    defun<1>(vm, name, f, TypeMaskList<T...>::doit());
    sq_pop(vm, 1);
}

inline
void defraw(
    HSQUIRRELVM vm, const HSQOBJECT& table,
    const std::string& name, SQInteger (*f)(HSQUIRRELVM)) {
    
    //sq_pushroottable(vm);
    sq_pushobject(vm, table);
    sq_pushstring(vm, name.data(), name.length());
    sq_newclosure(vm, f, 0);
    sq_setnativeclosurename(vm, -1, name.c_str());
    sq_newslot(vm, -3, SQFalse);
    sq_pop(vm, 1);
}

}

}

#endif // SQUALL_DEFUN_HPP_
