<mxfile host="app.diagrams.net">
  <diagram name="PET-01 UML" id="pet01-fixed">
    <mxGraphModel dx="1200" dy="800" grid="1" gridSize="10" guides="1" tooltips="1" connect="1" arrows="1" fold="1" page="1" pageScale="1" pageWidth="850" pageHeight="1100" math="0" shadow="0">
      <root>
        <mxCell id="0"/>
        <mxCell id="1" parent="0"/>

        <mxCell id="pet" value="&lt;b&gt;&amp;lt;&amp;lt;abstract&amp;gt;&amp;gt;&lt;/b&gt;&lt;br&gt;&lt;font style=&quot;font-size:24px&quot;&gt;Pet&lt;/font&gt;&lt;hr&gt;- id : int&lt;br&gt;- name : std::string&lt;br&gt;- level : int&lt;br&gt;- health : int&lt;br&gt;- happiness : int&lt;br&gt;- hunger : int&lt;br&gt;- energy : int&lt;hr&gt;+ eat() : void&lt;br&gt;+ play() : void&lt;br&gt;+ sleep() : void&lt;br&gt;+ bath() : void&lt;br&gt;+ getType() : std::string"
                 style="rounded=0;whiteSpace=wrap;html=1;fillColor=#FFFFFF;strokeColor=#000000;fontSize=16;align=left;spacingLeft=12;spacingTop=10;"
                 vertex="1" parent="1">
          <mxGeometry x="300" y="80" width="330" height="430" as="geometry"/>
        </mxCell>

        <mxCell id="range" value="&lt;b&gt;Khoảng giá trị chỉ số&lt;/b&gt;&lt;br&gt;health : 0–100&lt;br&gt;happiness : 0–100&lt;br&gt;hunger : 0–100&lt;br&gt;energy : 0–100"
                 style="rounded=1;whiteSpace=wrap;html=1;fillColor=#EAF4FF;strokeColor=#3399FF;fontSize=16;align=left;spacingLeft=12;spacingTop=10;"
                 vertex="1" parent="1">
          <mxGeometry x="40" y="190" width="220" height="150" as="geometry"/>
        </mxCell>

        <mxCell id="cat" value="&lt;b&gt;Cat&lt;/b&gt;&lt;hr&gt;+ play() : void&lt;br&gt;+ getType() : std::string"
                 style="rounded=0;whiteSpace=wrap;html=1;fillColor=#EFFFF0;strokeColor=#00AA44;fontSize=16;align=left;spacingLeft=12;spacingTop=10;"
                 vertex="1" parent="1">
          <mxGeometry x="150" y="610" width="260" height="130" as="geometry"/>
        </mxCell>

        <mxCell id="dog" value="&lt;b&gt;Dog&lt;/b&gt;&lt;hr&gt;+ play() : void&lt;br&gt;+ getType() : std::string"
                 style="rounded=0;whiteSpace=wrap;html=1;fillColor=#FFF4E8;strokeColor=#FF9933;fontSize=16;align=left;spacingLeft=12;spacingTop=10;"
                 vertex="1" parent="1">
          <mxGeometry x="520" y="610" width="260" height="130" as="geometry"/>
        </mxCell>

        <mxCell id="edgeCat" value="" style="edgeStyle=orthogonalEdgeStyle;rounded=0;orthogonalLoop=1;jettySize=auto;html=1;endArrow=block;endFill=0;"
                 edge="1" parent="1" source="cat" target="pet">
          <mxGeometry relative="1" as="geometry"/>
        </mxCell>

        <mxCell id="edgeDog" value="" style="edgeStyle=orthogonalEdgeStyle;rounded=0;orthogonalLoop=1;jettySize=auto;html=1;endArrow=block;endFill=0;"
                 edge="1" parent="1" source="dog" target="pet">
          <mxGeometry relative="1" as="geometry"/>
        </mxCell>
      </root>
    </mxGraphModel>
  </diagram>
</mxfile>