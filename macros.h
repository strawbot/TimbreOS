
/* Macros for botKernel  Rob Chapman  Mar 21, 1995 */
// Copyright TimbreWorks Engineering 1990-2005

#define _TOP_  sp[0]
#define _NEXT_  sp[1]
#define _TOS_  sp[2]
#define _CELL_  *--sp = sizeof(Cell)
#define _CELLS_  _TOP_ *= sizeof(Cell)
#define _SWAP_  *--rp = _NEXT_, _NEXT_ = _TOP_, _TOP_ = *rp++
#define _DROP_  sp++
#define _DUP_  --sp, _TOP_ = _NEXT_
#define _NUP_  _DUP_, _NEXT_ = _TOS_
#define _NIP_  _NEXT_ = _TOP_, _DROP_
#define _TUCK_  _DUP_, _NEXT_ = _TOS_, _TOS_ = _TOP_
#define _OVER_  --sp, _TOP_ = _TOS_
#define _2DROP_  sp += 2
#define _2DUP_  _OVER_, _OVER_
#define _RFROM_  *--sp = *rp++
#define _TOR_  *--rp = *_DROP_
#define _R_   *--sp = *rp
#define _EXECUTE_ tick = (void (**)())(*_DROP_), (**tick)()
#define _AND_  _NEXT_ &= _TOP_, _DROP_
#define _OR_  _NEXT_ |= _TOP_, _DROP_
#define _XOR_  _NEXT_ ^= _TOP_ ,_DROP_
#define _NOT_  _TOP_ = ~_TOP_
#define _2STAR_  _TOP_ <<= 1
#define _2SLASH_ _TOP_ >>= 1
#define _U2SLASH_ _TOP_ >>= 1
#define _PLUS_  _NEXT_ += _TOP_, _DROP_
#define _MINUS_  _NEXT_ -= _TOP_, _DROP_
#define _NEGATE_ _TOP_ = -(_TOP_)
#define _ABS_  if((Integer)_TOP_ < 0) _NEGATE_
#define _MAX_  if((Integer)_TOP_ > (Integer)_NEXT_) _NEXT_=_TOP_; _DROP_
#define _MIN_  if((Integer)_TOP_<(Integer)_NEXT_) _NEXT_=_TOP_; _DROP_
#define _STAR_  _NEXT_ *= _TOP_, _DROP_
#define _SLASH_  _NEXT_ /= _TOP_, _DROP_
#define _MOD_  _NEXT_ %= _TOP_, _DROP_
#define _SLASHMOD_ { Whole q = _NEXT_/_TOP_; _NEXT_ %= _TOP_; _TOP_ = q;}
#define _FETCH_  _TOP_ = *(Cell *)(_TOP_)
#define _CFETCH_ _TOP_ = (Cell)(*(Byte *)(_TOP_))
#define _WFETCH_ _TOP_ = (Cell)(*(unsigned short *)(_TOP_))
#define _STORE_  *(Cell *)(_TOP_) = _NEXT_, _2DROP_
#define _CSTORE_ *(Byte *)(_TOP_) = (Byte)(_NEXT_), _2DROP_
#define _WSTORE_ *(unsigned short *)(_TOP_) = (unsigned short)(_NEXT_), _2DROP_
#define _PLUSSTORE_ *(Cell *)(_TOP_) += _NEXT_, _2DROP_
#define _EQUALS_ if(_NEXT_ == _TOP_) *++sp = (Long)-1; else *++sp = 0
#define _0EQUALS_ _TOP_= _TOP_ == 0 ? (Long)-1 : 0
#define _LESSTHAN_ if((Integer)_NEXT_ < (Integer)_TOP_) *++sp = (Long)-1; else *++sp = 0
#define _GREATERTHAN_ if((Integer)_NEXT_ > (Integer)_TOP_) *++sp = (Long)-1; else *++sp = 0
#define _ULESSTHAN_ if(_NEXT_ < _TOP_) *++sp=(Long)(Long)-1; else *++sp=0
#define _UGREATERTHAN_ if(_NEXT_ > _TOP_) *++sp=(Long)-1; else *++sp=0
#define _COUNT_ --sp, _TOP_ = *((Byte *)_NEXT_)++
#define _CFETCH_PLUS_ _DUP_, _NEXT_ = *((Byte *)_TOP_)++
#define _FOR_
#define lit(n) *--sp=(Cell)(n)
#define ret (*_DROP_)
