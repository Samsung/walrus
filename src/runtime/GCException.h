/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WalrusGCException__
#define __WalrusGCException__

#include "runtime/GCBase.h"

namespace Walrus {

class Exception;

class GCException : public GCBase {
public:
    static GCException* exceptionNew(std::unique_ptr<Exception>& e);

    void throwException();

    std::unique_ptr<Exception>& exception()
    {
        return m_exception;
    }

private:
    GCException(std::unique_ptr<Exception>& e);

    std::unique_ptr<Exception> m_exception;
};

} // namespace Walrus

#endif // __WalrusGCException__
