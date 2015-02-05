// C compiler check for structure size
// from: http://bytes.com/topic/c/answers/220022-if-sizeof

#define _x_CCASERT_LINE_CAT(predicate, line) typedef char constraint_violated_on_line_##line[2*((predicate) != 0)-1];
#define CCASSERT(predicate) _x_CCASERT_LINE_CAT(predicate, __LINE__)

// Usage: CCASSERT(1) to pass; CCASSERT(0) to fail
/*
	typedef struct {
		long x;
		long y;
	}foo ;
	CCASSERT(sizeof(foo) < 10) // will not complain
*/