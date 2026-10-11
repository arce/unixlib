// cerrojo.h: bloqueo de regiones de un archivo, igual en Unix y en Windows
#ifndef CERROJO_H
#define CERROJO_H

#ifdef _WIN32
#include <io.h>
#include <string.h>
#include <windows.h>

// Bloquea 'len' bytes desde 'inicio' (len = 0: hasta el final). Espera si está ocupado.
static inline int bloquear(int fd, long inicio, long len) {
  OVERLAPPED ov;
  memset(&ov, 0, sizeof ov);
  ov.Offset = (DWORD)inicio;
  DWORD bajo = len ? (DWORD)len : 0xFFFFFFFF, alto = len ? 0 : 0xFFFFFFFF;
  return LockFileEx((HANDLE)_get_osfhandle(fd), LOCKFILE_EXCLUSIVE_LOCK, 0, bajo, alto, &ov) ? 0 : -1;
}
static inline int desbloquear(int fd, long inicio, long len) {
  OVERLAPPED ov;
  memset(&ov, 0, sizeof ov);
  ov.Offset = (DWORD)inicio;
  DWORD bajo = len ? (DWORD)len : 0xFFFFFFFF, alto = len ? 0 : 0xFFFFFFFF;
  return UnlockFileEx((HANDLE)_get_osfhandle(fd), 0, bajo, alto, &ov) ? 0 : -1;
}

#else
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static inline int bloquear(int fd, long inicio, long len) {
  struct flock fl;
  memset(&fl, 0, sizeof fl);
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = inicio;
  fl.l_len = len; // 0 = hasta el final del archivo
  return fcntl(fd, F_SETLKW, &fl);
}
static inline int desbloquear(int fd, long inicio, long len) {
  struct flock fl;
  memset(&fl, 0, sizeof fl);
  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = inicio;
  fl.l_len = len;
  return fcntl(fd, F_SETLKW, &fl);
}
#endif

#endif
