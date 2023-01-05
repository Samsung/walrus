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
#include "Walrus.h"

#include "Exception.h"

namespace Walrus {

Exception::Exception(ExecutionState& state)
{
    Optional<ExecutionState*> s = &state;

    while (s) {
        if (s->m_programCounterPointer) {
            m_programCounterInfo.pushBack(std::make_pair(s.value(), *s->m_programCounterPointer.value()));
        }
        s = s->m_parent;
    }
}

} // namespace Walrus
