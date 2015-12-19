#include <nan.h>
#include <node.h>
#include <stdio.h>

#include <memory>
#include <string>
#include <vector>

#include "argon2/src/argon2.h"

namespace {

const auto ENCODED_LEN = 108u;
const auto HASH_LEN = 32u;
const auto SALT_LEN = 16u;

class EncryptAsyncWorker : public Nan::AsyncWorker {
public:
    EncryptAsyncWorker(Nan::Callback* callback,
        const std::string& plain, const std::string& salt);

    void Execute();

    void HandleOKCallback();

private:
    std::string plain;
    std::string salt;
    std::string error;
    std::string output;
};

EncryptAsyncWorker::EncryptAsyncWorker(Nan::Callback* callback,
    const std::string& plain, const std::string& salt) :
    Nan::AsyncWorker(callback), plain{plain},
    salt{salt}, error{}, output{}
{
    if (salt.size() < SALT_LEN) {
        this->salt.resize(SALT_LEN, 0x0);
    }
}

void EncryptAsyncWorker::Execute()
{
    char encoded[ENCODED_LEN];

    auto result = argon2i_hash_encoded(3, 4096, 1, plain.c_str(), plain.size(),
        salt.c_str(), salt.size(), HASH_LEN, encoded, ENCODED_LEN);
    if (result != ARGON2_OK) {
        return;
    }

    output = std::string{encoded};
}

void EncryptAsyncWorker::HandleOKCallback()
{
    using v8::Local;
    using v8::Value;

    Nan::HandleScope scope;

    Local <Value> argv[2];
    argv[0] = Nan::Undefined();
    argv[1] = Nan::Encode(output.c_str(), output.size(), Nan::BINARY);

    callback->Call(2, argv);
}

NAN_METHOD(Encrypt) {
    using v8::Function;
    using v8::Local;

    Nan::HandleScope scope;

    if (info.Length() < 3) {
        Nan::ThrowTypeError("3 arguments expected");
        return;
    }

    Nan::Utf8String plain{info[0]->ToString()};
    Nan::Utf8String salt{info[1]->ToString()};
    Local<Function> callback = Local<Function>::Cast(info[2]);

    auto worker = new EncryptAsyncWorker(new Nan::Callback(callback), *plain, *salt);

    Nan::AsyncQueueWorker(worker);
}

}

NAN_MODULE_INIT(init) {
    Nan::Export(target, "encrypt", Encrypt);
};

NODE_MODULE(argon2_lib, init);
