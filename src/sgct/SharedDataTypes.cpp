/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/shareddatatypes.h>

#include <sgct/mutexmanager.h>

namespace sgct {

template class SharedObject<float>;
template class SharedObject<double>;
template class SharedObject<int64_t>;
template class SharedObject<uint64_t>;
template class SharedObject<int32_t>;
template class SharedObject<uint32_t>;
template class SharedObject<int16_t>;
template class SharedObject<uint16_t>;
template class SharedObject<int8_t>;
template class SharedObject<bool>;
template class SharedObject<std::string>;
template class SharedObject<std::wstring>;

} // namespace sgct
