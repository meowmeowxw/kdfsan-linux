#include "kdfsan_types.h"
#include "kdfsan_internal.h"
#include "kdfsan_interface.h"

dfsan_label __dfsan_arg_tls[64] = { -1 }; // should be { 0 }! this is correctly initialized in kdf_preinit_data()!
dfsan_label __dfsan_retval_tls = -1; // should be 0! this is correctly initialized in kdf_preinit_data()!
bool kdf_is_init_done = -1; // should be false! this is correctly initialized in kdf_preinit_data()!
bool kdf_is_in_rt = -1; // should be false! this is correctly initialized in kdf_preinit_data()!

/**** Checks for whether shadow mem can be accessed ****/

// Set after init routine
void kdf_init_finished(void) { kdf_is_init_done = true; }

// Check for whether dfsan rt is being called from a function called by dfsan rt
void set_rt(void) { kdf_is_in_rt = true; }
void unset_rt(void) { kdf_is_in_rt = false; }

/**** Interfaces inserted by pass ****/

dfsan_label __dfsan_read_label(const void *addr, uptr size) {
  if (size == 0) return 0;
  ENTER_RT(0);
  dfsan_label ret = kdf_read_label(addr,size);
  KDF_CHECK_LABEL(ret);
  LEAVE_RT();
  return ret;
}

void __dfsan_set_label(dfsan_label label, void *addr, uptr size) {
  if(size == 0) return;
  ENTER_RT();
  KDF_CHECK_LABEL(label);
  kdf_set_label(label,addr,size);
  LEAVE_RT();
}

dfsan_label __dfsan_union(dfsan_label l1, dfsan_label l2) {
  if (l1 == 0) return l2;
  if (l2 == 0) return l1;
  if (l1 == l2) return l1;
  ENTER_RT(0);
  KDF_CHECK_LABEL(l1); KDF_CHECK_LABEL(l2);
  dfsan_label ret = kdf_union(l1,l2);
  KDF_CHECK_LABEL(ret);
  LEAVE_RT();
  return ret;
}

void __dfsan_vararg_wrapper(const char *fname) {
  ENTER_RT();
  printk("KDFSan ERROR: unsupported indirect call to vararg\n");
  LEAVE_RT();
}

/**** Callback interfaces inserted by pass ****/

dfsan_label __dfsan_load_callback(void *addr, uptr size, dfsan_label data_label, dfsan_label ptr_label) {
  kdfinit_access_taint_sink(addr, size, _RET_IP_, data_label, ptr_label, false);
  ENTER_RT(data_label);
  KDF_CHECK_LABEL(data_label); KDF_CHECK_LABEL(ptr_label);
  dfsan_label policy_label = kdfinit_load_taint_source(addr, size, _RET_IP_, data_label, ptr_label);
  dfsan_label ret_label = kdf_union(data_label, policy_label);
  LEAVE_RT();
  return ret_label;
}

dfsan_label __dfsan_store_callback(void *addr, uptr size, dfsan_label data_label, dfsan_label ptr_label) {
  kdfinit_access_taint_sink(addr, size, _RET_IP_, data_label, ptr_label, true);
  return data_label;
}

void __dfsan_mem_transfer_callback(void *dest, const void *src, uptr size) {
  if(size == 0) return;
  ENTER_RT();
  kdf_memtransfer(dest, src, size);
  LEAVE_RT();
}

void __dfsan_cmp_callback(dfsan_label combined_label) { }

/**** Interfaces not inserted by pass ****/

void dfsan_add_label(dfsan_label label_src, void *addr, uptr size) {
  if(size == 0 || label_src == 0) return;
  ENTER_RT();
  KDF_CHECK_LABEL(label_src);
  kdf_add_label(label_src,addr,size);
  LEAVE_RT();
}

// TODO: userdata unused; remove
dfsan_label dfsan_create_label(const char *desc, void *userdata) {
  ENTER_RT(0);
  dfsan_label ret = kdf_create_label(desc);
  KDF_CHECK_LABEL(ret);
  LEAVE_RT();
  return ret;
}

int dfsan_has_label(dfsan_label label, dfsan_label elem) {
  if (label == elem) return true;
  ENTER_RT(false);
  KDF_CHECK_LABEL(label); KDF_CHECK_LABEL(elem);
  int ret = kdf_has_label(label,elem);
  LEAVE_RT();
  return ret;
}

dfsan_label dfsan_has_label_with_desc(dfsan_label label, const char *desc) {
  ENTER_RT(0);
  KDF_CHECK_LABEL(label);
  dfsan_label ret = kdf_has_label_with_desc(label,desc);
  KDF_CHECK_LABEL(ret);
  LEAVE_RT();
  return ret;
}

dfsan_label dfsan_get_label_count(void) {
  ENTER_RT(0);
  dfsan_label ret = kdf_get_label_count();
  LEAVE_RT();
  return ret;
}

dfsan_label dfsan_read_label(const void *addr, uptr size) {
  return __dfsan_read_label(addr, size);
}

void dfsan_set_label(dfsan_label label, void *addr, uptr size) {
  __dfsan_set_label(label, addr, size);
}

dfsan_label dfsan_union(dfsan_label l1, dfsan_label l2) {
  return __dfsan_union(l1,l2);
}

dfsan_label __dfsw_dfsan_get_label(long data, dfsan_label data_label, dfsan_label *ret_label) {
  *ret_label = 0;
  return data_label;
}

void dfsan_mem_transfer_callback(void *dest, const void *src, uptr size) {
  __dfsan_mem_transfer_callback(dest, src, size);
}

/**** Misc. interfaces ****/

void dfsan_copy_label_info(dfsan_label label, char * dest, size_t count) {
  ENTER_RT();
  KDF_CHECK_LABEL(label);
  kdf_copy_label_info(label, dest, count);
  LEAVE_RT();
}
