# Maps

Key-value collections with string keys. Insertion order is preserved.

## Literal

```
var empty = {};
var user = {"name": "Alice", "age": 30};
```

Keys must be strings. Values can be any type.

## Access

```
var m = {"x": 10, "y": 20};
print m["x"];                 // 10
m["z"] = 30;
print m;                      // {x: 10, y: 20, z: 30}
```

## Iteration

```
var config = {"debug": true, "verbose": false};
for entry in config {
    print entry;
}
```

## Nested Maps

```
var data = {
    "user": {
        "name": "Mac",
        "scores": [10, 20, 30]
    }
};
print data["user"]["name"];   // Mac
```

## See Also

- [Arrays](./arrays.md)
- [Types](./types.md)
