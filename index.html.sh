cat <<EOF

<html>
<head>
<title>The "g" graph language</title>
</head>
<body>

<h1>Introduction</h1>

<pre>
The "DOT" language is amazingly concise at describing graphs.  The "g" language is my attempt at doing at least as
well while adding some extra capabilities.

The goals of "g" are to:

0. represent any graph that "DOT" can.

1. incremental graph creation, modifications through a stream of "acts"
  - no beginning, no end,  the "graph" is what you see as a result.
  - Allow for and encourage a multi-threaded implementation.

2. rethink of Subgraphs:
  - Subgraphs used to to describe sets of objects to attatch attributes to
  - Subgraphs used to describe clusters
   
    2a) use of string partterns to specify meta-object and class-like objects

    2b) provide general containment to subsume clusters (i.e.a cluster becomes a first-class node in 'g')
      - Any node can contain a graph
      - Any edge can contain a graph

3. netlists.  (i.e. edges with more than two ends)

4. "multi-nodes" analogous to "multi-edges"

6. minimize special characters:  '-' is not a special character, so now number strings can be used 
   as simple strings.  

7. simplify:  insist that ids use only simple strings with no escapes or quotes.
</pre>

<h1>Table of Contents</h1>

( Note. Some of these pages are generated automatically during the build process.   Please  build
before viewing. )


<ol>
<li>How to build "g"
<ul>
<li> Type "make" in the top-directory (i.e. in the same directory as this index.html).
</ul>
<li>The Grammar of "g":
<ul>
<li><a type="text/plain" href="doc/annotated_grammar.ebnf">Annotated eBNF (not definitive)</a>
<li><a type="text/plain" href="src/grammar.g">The definitive source in "g"</a>
<li><a type="text/plain" href="src/grammar.ebnf">Rendered in eBNF from definitive source</a>
<li><a href="src/grammar.svg">Rendered by dot from definitive source</a>
</ul>
<li>Some hand drawn examples:
<ul>
<li><a href="doc/sketches/hello_world.svg">hello_world</a>
<li><a href="doc/sketches/edge_into_container_using_nodepath.svg">edge_into_container_using_nodepath</a>
<li><a href="doc/sketches/edge_into_container_via_port.svg">edge_into_container_via_port</a>
<li><a href="doc/sketches/netlist.svg">netlist</a>
<li><a href="doc/sketches/implement_netlist_with_edge_container.svg">implement_netlist_with_edge_container</a>
<li><a href="doc/sketches/multi_node_multi_edge.svg">multi_node_multi_edge</a>
</ul>
<li><a href="src/">Sources</a>
<ul>
EOF

shopt -s nullglob
for i in src/Makefile src/*.c src/*.sh;do
   echo "<li><a type=\"text/plain\" href=\"$i\">${i#src/}</a>"
   for j in ${i%.c}.[hg]; do
       echo "<a type=\"text/plain\" href=\"$j\">${j#src/}</a>"
   done
done

cat <<EOF
</ul>
<li><a href="src/">Example Graphs</a>
<ul>
EOF

for i in graphs/*.g;do
   echo "<li><a type=\"text/plain\" href=\"$i\">${i#graphs/}</a>"
done

cat <<EOF
</ul>
</ol>

</body>
</html>
EOF
