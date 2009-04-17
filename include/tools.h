#ifndef __TOOLS_H_DEFINED__
#define __TOOLS_H_DEFINED__

template<class T, std::size_t D>
inline std::size_t ArraySize(const T (&)[D])
{
  return D;
}

template<class T, std::size_t D>
inline const T* ArrayEnd(const T (&c)[D])
{
  return c + D;
}

template<class T, std::size_t D>
inline T* ArrayEnd(T (&c)[D])
{
  return c + D;
}

template<class T, class F>
inline T safe_ptr_cast(F* from)
{
  return static_cast<T>(static_cast<void*>(from));
}

template<class T, class F>
inline T safe_ptr_cast(const F* from)
{
  return static_cast<T>(static_cast<const void*>(from));
} 

#endif //__TOOLS_H_DEFINED__
