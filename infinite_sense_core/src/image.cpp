
#include "image.h"
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <mutex>
namespace infinite_sense {
#ifndef CV_XADD
static std::mutex mutex;
int CvXAdd(int* addr, const int delta) {
  std::unique_lock lock(mutex);
  const int tmp = *addr;
  *addr += delta;
  return tmp;
}
#endif

GMat::GMat() : cols(0), rows(0), flags(0), data(nullptr), ref_count(nullptr) {}

GMat::GMat(const int rows_, const int cols_, const int type, uchar* src, const bool copy, const int image_align)
    : cols(cols_), rows(rows_), flags(type), data(nullptr), ref_count(nullptr) {
  if (src && !copy) {
    data = src;
    return;
  }

  const int byte_num = Total() * ElemSize();
  if (byte_num <= 0) return;

  const int align_bytes = AlignSize(byte_num, sizeof(*ref_count));
  data = static_cast<uchar*>(FastMalloc(align_bytes + sizeof(int*), image_align));

  if (!data) {
    cols = 0;
    rows = 0;
    return;
  }

  ref_count = reinterpret_cast<int*>(data + align_bytes);
  *ref_count = 1;

  if (src) {
    memcpy(data, src, byte_num);
  }
}

GMat::GMat(const GMat& ref)
    : cols(ref.cols), rows(ref.rows), flags(ref.flags), data(ref.data), ref_count(ref.ref_count) {
  if (ref_count) CvXAdd(ref_count, 1);
}

GMat::~GMat() {
  if (data && ref_count) {
    if (CvXAdd(ref_count, -1) == 1) {
      Release();
    }
  }
  cols = rows = flags = 0;
  data = nullptr;
  ref_count = nullptr;
}

GMat& GMat::operator=(const GMat& rhs) {
  this->~GMat();
  cols = rhs.cols;
  rows = rhs.rows;
  flags = rhs.flags;
  data = rhs.data;
  ref_count = rhs.ref_count;
  if (ref_count) CvXAdd(ref_count, 1);
  return *this;
}

void GMat::Release() {
  const size_t total_bytes = Total() * ElemSize();
  const int align_bytes = AlignSize(total_bytes, (int)sizeof(*ref_count));
  if (ref_count == reinterpret_cast<int*>(data + align_bytes))  // OpenCV2 style
  {
    cols = rows = 0;
    FastFree(data);
    ref_count = nullptr;
    data = nullptr;
  } else  // OpenCV3 style
  {
    if (auto* u = reinterpret_cast<UMatData*>(reinterpret_cast<uchar*>(ref_count) - offsetof(UMatData, refcount));
        u->userdata == data + align_bytes) {
      // this is allocated by GMat, we need minus both refcount
      ref_count = static_cast<int*>(u->userdata);
      if (CvXAdd(ref_count, -1) == 1) {
        return Release();
      }
    } else {
      assert(u->size == total_bytes);
      Deallocate(u);
    }
    cols = rows = 0;
    ref_count = nullptr;
    data = nullptr;
  }
}

GMat GMat::Create(int rows, int cols, int type, uchar* src, bool copy) { return {rows, cols, type, src, copy}; }

GMat GMat::Zeros(const int rows, const int cols, const int type, uchar* src, const bool copy, const int image_align) {
  GMat result(rows, cols, type, src, copy, image_align);
  memset(result.data, 0, result.Total() * result.ElemSize());
  return result;
}

template <typename Tp>
Tp* GMat::AlignPtr(Tp* ptr, int n) {
  return (Tp*)(((size_t)ptr + n - 1) & -n);
}

void* GMat::FastMalloc(const size_t size, const int image_align) {
  auto* udata = static_cast<uchar*>(malloc(size + sizeof(void*) + image_align));
  if (!udata) {
    return nullptr;
  }
  uchar** adata = AlignPtr(reinterpret_cast<uchar**>(udata) + 1, image_align);
  adata[-1] = udata;
  return adata;
}

void GMat::FastFree(void* ptr) {
  if (ptr) {
    uchar* udata = static_cast<uchar**>(ptr)[-1];
    free(udata);
  }
}

size_t GMat::AlignSize(const size_t sz, const int n) {
  assert((n & (n - 1)) == 0);  // n is a power of 2
  return (sz + n - 1) & -n;
}

void GMat::Deallocate(UMatData* u)  // for OpenCV Version 3
{
  if (!u) {
    return;
  }
  assert(u->ref_count == 0);
  assert(u->refcount == 0);
  if (!(u->flags & UMatData::USER_ALLOCATED)) {
    const uchar* udata = reinterpret_cast<uchar**>(u->orig_data)[-1];
    if (const int n = u->orig_data - udata; (n & (n - 1)) == 0 && n >= 0 && n <= 32) {
      FastFree(u->orig_data);
    } else
      free(u->orig_data);
    u->orig_data = nullptr;
  }
  delete u;
}
}  // namespace infinite_sense