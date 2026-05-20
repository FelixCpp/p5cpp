# Ablauf

### Szenario 1

1. Benutzer: "Hallo"
2. Benutzer: "Hallo"
3. Benutzer -> Upload

> Upload "Hallo"

### Szenario 2

1. Benutzer: "Hallo"
2. Benutzer -> Upload
3. Benutzer: "Hallo"
4. Benutzer -> Upload

> Upload "Hallo"

### Szenario 3

1. Benutzer: "Hallo"
2. Benutzer -> Upload
3. Benutzer -> Upload

> Upload "Hallo"

### Szenario 4

1. Benutzer: "Hallo"
2. Benutzer -> Upload
3. Benutzer: "Welt"
4. Benutzer -> Upload

> Upload "Hallo"

> Upload "Welt"

### Szenario 5

1. Benutzer: "Hallo"
2. Benutzer -> Upload
3. Benutzer: "Welt"
4. Benutzer -> Upload
5. Benutzer: "Hallo"
6. Benutzer -> Upload

> Upload "Hallo"

> Upload "Welt"

> Upload "Hallo"

### Uniform

- Typ (Enum)
- Wert (Typ)
- Dirty (Boolean)

### Uniforms

- Name (String)
- Location (Int)
- Typ (Uniform)

- Id (Int)
- [Uniforms]

# Upload

> Id
>
> > Cache: Dirty Einträge via Id holen
> > Einträge hochladen

### Cache

1. Id > Alle Dirty einträge querien
2. Id > Alle Dirty einträge "undurtien"
