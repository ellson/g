- documentation
	- self documenting as much as possible
	- retain comment in grammar.g from "g -d"  ???
		- but would increase size of binary
	- have gacc.sh put more comments in grammar.c
	- have gacc.sh print grammar.ebnf ???
	- README.md
	- g.man
	- .... extended discussion paper ?
- fix parser 
	DONE - separate state and prop into parallel arrays
	- deal with terminations
		- single char token
		- frags
		- two char tokens
- in eBNF show chars as chars when ABC or terminal tokens
- canonicalize subject
	- retain order
	- expand  endpointsets
	- collapse lists with single element
- support SAMEEND
- patterns
	- pattern syntax
	- pattern matching
- buffer management
	- minimize need for malloc()
	- ref count or equiv from frags
	- free reference to  SAMEND string as soon as possible
	- know when all refs have gone, then reuse
- TWO character tokens
	- EOL
	- ESC
	- CMNTs
		- /* ... */
		- // ... EOL
		- /? meta event ... EOL
			- convert to regular comment ?
			-  // /? ... EOL
			- or always ignore ?
		- # ... EOL
	- EDGEOP
		- ->
		- --
- WS handling 
	- CRLF
	- SREP
	- EOL counting
- Other quoting mechanisms
	- HTML-like
	- Content-Length:
	- multipart
- Meta state events:
	- pattern_match
	- end_of_statement
	- end_of_sameend_statements
	- syntax in output ???  some kind of comment
	- should not be needed for <g g> networks
- errors
	- syntax errors
		- line and char pos where error detected
		- context of error
		- expected input (eBNF ?)
	- string errors
		- quote mismatch
	- meta errors
		- non-homogenous list
		- buffer space
		- end_of_stream
- comment frag
	- permit retention of comments in order
	- retain introducers and eol if comment terminator
- whitespace
		-- whitespace not retained?
		-- whitepace in output depends on prettyprint flags
- Emitter
	- deal with pretty printing
		- minimal
		- pretty
		- shell friendly
			-- with or without meta events
	- option to retain comments
	- option to print in
		- canonical (expanded endsets)
		- or with endsets
- Options
	- minimal | pretty | shell

- Environment
	- Container props ?
	- stream stats
	- meta event stats
	- stream liveness (watchdogs? alternates? HA?)
	- stream graph

- communcating state machines?