#ifndef FIREBLOOM_H
#define FIREBLOOM_H

#if WITH_FIREBLOOM

#define FIREBLOOM_PTR_DECL(data_type, name) \
    data_type name##_raw, data_type name##_lower, data_type name##_upper, void *name##_type

#define FIREBLOOM_PTR(raw, lower, upper, type) \
    raw, lower, upper, (void *)type

#define FIREBLOOM_PTR_CASTED(new_type, raw, lower, upper, type) \
    (new_type)(raw), (new_type)(lower), (new_type)(upper), (void *)type

#define FIREBLOOM_UNWRAP(name) \
    name##_raw, name##_lower, name##_upper, name##_type

#define FIREBLOOM_NULL_PTR  NULL, NULL, NULL, NULL

#else

#define FIREBLOOM_PTR_DECL(data_type, name) \
    data_type name##_raw

#define FIREBLOOM_PTR(raw, lower, upper, type) \
    raw

#define FIREBLOOM_PTR_CASTED(new_type, raw, lower, upper, type) \
    (new_type)(raw)

#define FIREBLOOM_UNWRAP(name) \
    name##_raw

#define FIREBLOOM_NULL_PTR  NULL

#endif

#endif
