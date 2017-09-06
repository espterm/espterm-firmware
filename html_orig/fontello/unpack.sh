#!/bin/bash

# resolve current file's directory
DIR=$(dirname $(realpath $0))

OUTPUT_DIR="$DIR/out"
SASS_DIR="$DIR/../sass"

ICON_PREFIX='icn'

# list with full paths, sort from newest
NEWEST=$(ls -dt1 "$DIR"/*.zip | head -1)

if [[ -z "$NEWEST" ]]; then
  echo "Fontello zip not found."
  exit 1
fi

# Clean the output folder
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

echo "Unpacking fontello..."

unzip -ju "$NEWEST" -d "$OUTPUT_DIR"

echo "Patching paths in the fontello CSS..."

# Fix bad relative paths in the CSS
sed -i "s|\.\./font/|/fonts/|g" "$OUTPUT_DIR/"*.css

echo "Generating SASS file with icon codes..."

SASSFILE="$SASS_DIR/_fontello.scss"

echo -e "@charset \"UTF-8\";\n\n/* Fontello data, processed by the unpack script. */\n" > "$SASSFILE"

# Extract the base font-face style
#grep -Pazo "(?s)@font-face.*?normal;\n\}" "$OUTPUT_DIR/fontello.css" \
#	| sed 's/\x0//g' >> "$SASSFILE"

grep -Pazo "(?s)@font-face \{\n\s*font-family: 'fontello';\n\s*src: url\('data.*?woff'\)" "$OUTPUT_DIR/fontello-embedded.css" \
	| sed 's/\x0//g' >> "$SASSFILE"

echo -e ";\n}" >> "$SASSFILE"


grep -Pazo "(?s)$ICON_PREFIX-\"\]:before .*?\}" "$OUTPUT_DIR/fontello.css" \
	| sed 's/\x0//g' \
	| sed "s/$ICON_PREFIX-\"\]:before/\n\n%fontello-icon-base \{\n\&::before /g" \
	>> "$SASSFILE"
echo -e "\n}" >> "$SASSFILE"

echo -e "\n\n/* Fontello icon codes */" >> "$SASSFILE"
echo -n "\$icon-codes: (" >> "$SASSFILE"
sed -r "s|\.$ICON_PREFIX-([a-z0-9-]+):before \{ content: ('.*?');.*?$|\t\1: \2,|g" "$OUTPUT_DIR/fontello-codes.css" \
	| sed -r "s|@.*||g" >> "$SASSFILE"

echo -ne "\n);\n" >> "$SASSFILE"

echo -ne "\n/* Fontello classes */" >> "$SASSFILE"
cat "$OUTPUT_DIR/fontello-codes.css" \
	| sed -r 's/\/\*.+\*\///g' \
	| sed -r "s|@.*||g" \
	| sed 's/:before/::before/g' >> "$SASSFILE"

TAIL=$(cat <<ASDF


[class^="$ICON_PREFIX-"], [class*=" $ICON_PREFIX-"] {
	@extend %fontello-icon-base;
}

@mixin icon-base() {
	@extend %fontello-icon-base;
}

@mixin icon-content(\$icon-name) {
	&::before {
		content: map-get(\$icon-codes, \$icon-name);
	}
}

@mixin icon(\$icon-name) {
	@include icon-base();
	@include icon-content(\icon-name);
}
ASDF
)

echo "$TAIL" >> "$SASSFILE"

echo -e "\e[32mFontello ready\e[0m"
