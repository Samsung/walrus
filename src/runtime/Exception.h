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

#ifndef __WalrusException__
#define __WalrusException__

namespace Walrus {

class String;
class Exception : public gc {
public:
    // we should use exception value as NoGC since bdwgc cannot find thrown value
    static std::unique_ptr<Exception> create(String* m)
    {
        return std::unique_ptr<Exception>(new (NoGC) Exception(m));
    }

    String* message() const
    {
        return m_message;
    }

private:
    Exception(String* message)
        : m_message(message)
    {
    }

    String* m_message;
};

} // namespace Walrus

#endif // __WalrusException__
