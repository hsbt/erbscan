#include "malloc.h"
#include "ruby.h"
#include "intern.h"
#include "util.h"

#ifdef StringValue
#else
#define StringValue(obj) (TYPE(obj)==T_STRING) ? (obj) : (rb_str_to_str(obj))
#endif


VALUE cERBScanner;
ID id_text;
ID id_code;
ID id_code_percent;
ID id_code_put;
ID id_code_comment;

struct Scanner {
  int trim_mode;
  int percent;
  int explicit_trim;
};

static void erbscan_free(struct Scanner *ptr) {
  xfree(ptr);
}

#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
static VALUE erbscan_s_allocate(VALUE klass) {
  struct Scanner *ptr = ALLOC(struct Scanner);
  ptr->trim_mode     = 0;
  ptr->percent       = 0;
  ptr->explicit_trim = 0;
  return Data_Wrap_Struct(klass, 0, erbscan_free, ptr);
}
#else
static VALUE erbscan_s_new(VALUE klass) {
  VALUE obj;
  struct Scanner *ptr = ALLOC(struct Scanner);
  ptr->trim_mode     = 0;
  ptr->percent       = 0;
  ptr->explicit_trim = 0;
  obj = Data_Wrap_Struct(klass, 0, erbscan_free, ptr);
  rb_obj_call_init(obj, 0, 0);
  return obj;
}
#endif

static VALUE erbscan_initialize(VALUE self) {
  rb_call_super(0,0);
  return self;
}

static VALUE erbscan_text(VALUE self, VALUE obj) {
  return obj;
}

static VALUE erbscan_code(VALUE self, VALUE obj) {
  return obj;
}

static VALUE erbscan_code_put(VALUE self, VALUE obj) {
  return obj;
}

static VALUE erbscan_code_comment(VALUE self, VALUE obj) {
  return obj;
}

static VALUE erbscan_code_percent(VALUE self, VALUE obj) {
  return obj;
}

static VALUE erbscan_get_trim_mode(VALUE self) {
  struct Scanner *ptr;
  Data_Get_Struct(self, struct Scanner, ptr);
  return INT2FIX(ptr->trim_mode);
}

static VALUE erbscan_set_trim_mode(VALUE self, VALUE value) {
  struct Scanner *ptr;
  int trim_mode;
  
  Check_Type(value, T_FIXNUM);
  trim_mode = FIX2INT(value);

  if ((trim_mode<0) || (trim_mode>2)) {
    rb_raise(rb_eArgError, "invalid 'trim_mode'");
  }
  
  Data_Get_Struct(self, struct Scanner, ptr);
  ptr->trim_mode = trim_mode;
  
  return value;
}

static VALUE erbscan_get_percent(VALUE self) {
  struct Scanner *ptr;
  Data_Get_Struct(self, struct Scanner, ptr);
  return INT2FIX(ptr->percent);
}

static VALUE erbscan_set_percent(VALUE self, VALUE value) {
  struct Scanner *ptr;
  int percent;
  
  Check_Type(value, T_FIXNUM);
  percent = FIX2INT(value);

  if ((percent<0) || (percent>1)) {
    rb_raise(rb_eArgError, "invalid 'percent'");
  }
  
  Data_Get_Struct(self, struct Scanner, ptr);
  ptr->percent = percent;
  
  return value;
}

static VALUE erbscan_get_explicit_trim(VALUE self) {
  struct Scanner *ptr;
  Data_Get_Struct(self, struct Scanner, ptr);
  return INT2FIX(ptr->explicit_trim);
}

static VALUE erbscan_set_explicit_trim(VALUE self, VALUE value) {
  struct Scanner *ptr;
  int explicit_trim;
  
  Check_Type(value, T_FIXNUM);
  explicit_trim = FIX2INT(value);

  if ((explicit_trim<0) || (explicit_trim>1)) {
    rb_raise(rb_eArgError, "invalid 'explicit_trim'");
  }
  
  Data_Get_Struct(self, struct Scanner, ptr);
  ptr->explicit_trim = explicit_trim;
  
  return value;
}



static VALUE erbscan_scan(VALUE self, VALUE recv, VALUE str) {
  struct Scanner *sc;
  char *src;
  int rest;
  char ch;
  int head;
  char *content;
  char *dst;
  int len;
  ID method;
  
  str = StringValue(str);
  OBJ_FREEZE(str);
  
  src  = RSTRING(str)->ptr;
  rest = RSTRING(str)->len;
  
  Data_Get_Struct(self, struct Scanner, sc);
  
  content = xmalloc(rest);
  
  dst = content;
  len = 0;
  head   = 1;
  while(rest>0) {
    ch = *src;
    switch(ch) {
    case '<':
      if ((rest>0) && (*(src+1)=='%')) {
        if ((rest>1) && (*(src+2)=='%')) {
          *dst++ = '<';
          *dst++ = '%';
          len  += 2;
          src  += 3;
          rest -= 3;
        } else {
          src += 2;
          rest -= 2;
          
          if (rest==0) break;
          /* 1文字目のチェック */
          ch = *src++;
          rest--;
          switch(ch) {
          case '=':
            method = id_code_put;
            if (rest==0) break;
            ch = *src++;
            rest--;
            break;
          case '#':
            method = id_code_comment;
            if (rest==0) break;
            ch = *src++;
            rest--;
            break;
          case '-':
            method = id_code;
            if (sc->explicit_trim && (rest>0)) {
              int tmplen = len;
              while (tmplen>0) {
                ch = *(--dst);
                if ((ch==' ') || (ch=='\t')) {
                  tmplen--;
                  if (tmplen==0) {
                    len = tmplen;
                    break;
                  }
                } else if (ch='\n') {
                  len = tmplen;
                  break;
                } else {
                  break;
                }
              }
              
              ch = *src++;
              rest--;
            }
          default:
            method = id_code;
          }
          if (len>0) {
            rb_funcall(recv, id_text, 1, rb_str_new(content, len));
          }
          dst = content;
          len = 0;
          while(rest>0) {
            if (ch=='%') {
              if (rest>0) {
                if (*src=='>') {
                  src++;
                  rest--;
                  /* trim_mode処理 */
                  switch(sc->trim_mode) {
                  case 0:
                    head = 0;
                    break;
                  case 2:
                    if (head==0) break;
                  case 1:
                    if (rest>0) {
                      ch = *src;
                      if (ch=='\r') {
                        src++;
                        rest--;
                        if (rest>0) {
                          if (*src=='\n') {
                            src++;
                            rest--;
                          }
                        }
                        head = 1;
                      } else if (ch=='\n') {
                        src++;
                        rest--;
                        head = 1;
                      } else {
                        head = 0;
                      }
                    }
                    break;
                  }
                  break;
                } else if ((*src=='%') && (rest>1) && (*(src+1)=='>')) {
                  *dst++ = '%';
                  *dst++ = '>';
                  len += 2;
                  src += 3;
                  rest -= 3;
                  break;
                }
              }
              *dst++ = ch;
              len++;
            } else if (ch=='-') {
              if ((rest>1) && (sc->explicit_trim)) {
                if (*src=='%' && *(src+1)=='>') {
                  src+=2;
                  rest-=2;
                  if (rest>0) {
                    ch = *src;
                    if (ch=='\r') {
                      src++;
                      rest--;
                      if (rest>0) {
                        if (*src=='\n') {
                          src++;
                          rest--;
                        }
                      }
                      head = 1;
                    } else if (ch=='\n') {
                      src++;
                      rest--;
                      head = 1;
                    } else {
                      head = 0;
                    }
                  }
                  break;
                }
              }
              *dst++ = ch;
              len++;
            } else {
              *dst++ = ch;
              len++;
            }
            ch = *src++;
            rest--;
          }
          if (len>0) {
            rb_funcall(recv, method, 1, rb_str_new(content, len));
          }
          dst = content;
          len = 0;
        }
      } else {
        *dst++ = ch;
        len++;
        src++;
        rest--;
      }
      break;
    case '%':
      if (head && (rest>0) && sc->percent) {
        if (*(src+1)=='%') {
          *dst++ = ch;
          len++;
          src  += 2;
          rest -= 2;
        } else {
          if (len>0) {
            rb_funcall(recv, id_text, 1, rb_str_new(content, len));
          }
          dst = content;
          len = 0;
          rest--;
          src++;
          while(rest>0) {
            ch = *src++;
            rest--;
            *dst++ = ch;
            len++;
            if (ch=='\n') break;
          }
          if (len>0) {
            rb_funcall(recv, id_code_percent, 1, rb_str_new(content, len));
          }
          dst = content;
          len = 0;
        }
      } else {
        *dst++ = ch;
        len++;
        src++;
        rest--;
      }
      break;
    default:
      *dst++ = ch;
      len++;
      src++;
      rest--;
      break;
    }
    head = (ch=='\n');
  }
  if (len>0) {
    rb_funcall(recv, id_text, 1, rb_str_new(content, len));
  }
  
  xfree(content);
  
  return self;
}


void Init_erbscan() {
  ID id;
  
  id_text = rb_intern("text");
  id_code = rb_intern("code");
  id_code_percent = rb_intern("code_percent");
  id_code_put = rb_intern("code_put");
  id_code_comment = rb_intern("code_comment");
  
  cERBScanner = rb_define_class("ERBScanner", rb_cObject);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
  rb_define_alloc_func(cERBScanner, erbscan_s_allocate);
#else
  rb_define_singleton_method(cERBScanner, "new", erbscan_s_new, 0);
#endif
  rb_define_method(cERBScanner, "initialize", erbscan_initialize, 0);
  rb_enable_super(cERBScanner, "initialize");
  
  rb_define_method(cERBScanner, "scan", erbscan_scan, 2);
  
  rb_define_method(cERBScanner, "trim_mode",      erbscan_get_trim_mode,      0);
  rb_define_method(cERBScanner, "trim_mode=",     erbscan_set_trim_mode,      1);
  rb_define_method(cERBScanner, "percent",        erbscan_get_percent,        0);
  rb_define_method(cERBScanner, "percent=",       erbscan_set_percent,        1);
  rb_define_method(cERBScanner, "explicit_trim",  erbscan_get_explicit_trim,  0);
  rb_define_method(cERBScanner, "explicit_trim=", erbscan_set_explicit_trim,  1);
}
