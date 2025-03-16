const data = {
	name: "Inode 0",
	type: "inode",
	children: [
		{
			name: "Block 4139",
			type: "data",
			children: []
		},
		{
			name: "Block 4140",
			type: "pointer",
			children: [
				{
					name: "Block 4141",
					type: "data",
					children: [
						{
							name: "Block 4159",
							type: "data",
							children: []
						},
						{
							name: "Block 4152",
							type: "data",
							children: []
						},
					]
				},
				{
					name: "Block 4205",
					type: "data",
					children: [
						{
							name: "Block 4159",
							type: "data",
							children: []
						},
					]
				},
				{
					name: "Block 4149",
					type: "data",
					children: [
						{
							name: "Block 4159",
							type: "data",
							children: [
								{
									name: "Block 4159",
									type: "data",
									children: [

									]
								},

							]
						},
					]
				},
				{
					name: "Block 4159",
					type: "data",
					children: []
				}
			]
		}
	]
};

const scale = 75;
const width = 16 * scale, height = 9 * scale;
const margin = { top: 10, right: 10, bottom: 10, left: 100 };
const treeWidth = width - margin.left - margin.right;
const treeHeight = height - margin.top - margin.bottom;
const svg = d3.select(".container#inode-view > svg")
	.attr("viewBox", `0 0 ${width} ${height}`)
	// .attr("width", width)
	// .attr("height", height)
const g = svg.append("g").attr("transform", `translate(${margin.left}, ${margin.top})`);

const tree = d3.tree().size([treeHeight, treeWidth]);
const root = d3.hierarchy(data, d => d.children);
root.x0 = treeHeight / 2;
root.y0 = 0;
root.children.forEach(collapse);

update(root);

function collapse(d) {
	if (!d.children) return;
	d._children = d.children
	d._children.forEach(collapse)
	d.children = null
}

var i = 0, duration = 750;

function update(src) {
	// assign the x, y coords of each node
	const treeData = tree(root);

	// calculate the new tree layout
	const nodes = treeData.descendants();
	const links = treeData.descendants().slice(1);

	// Normalize for fixed-depth.
	nodes.forEach(d => { d.y = d.depth * 180});

	/* NODES */
	const nodeRect = { width: 60, height: 40 };
	nodeRect.dx = nodeRect.width / 2;
	nodeRect.dy = nodeRect.height / 2;
	const node = svg.select("g").selectAll("g.node").data(nodes, d => d.id || (d.id = ++i));
	const nodeEnter = node.enter().append("g")
		.attr("class", "node")
		.attr("transform", _ => `translate(${src.y0}, ${src.x0})`)
		.on("click", click);
	nodeEnter.append("rect").attr("class", "node").attr("fill", "lightgray").attr("stroke", "black");
	nodeEnter.append("text").text(d => d.data.name).attr("class", "inode-title");

	const nodeUpdate = nodeEnter.merge(node);
	nodeUpdate.transition()
		.duration(duration)
		.attr("transform", d => `translate(${d.y}, ${d.x})`);
	nodeUpdate.select("rect.node")
		.attr("width", nodeRect.width)
		.attr("height", nodeRect.height)
		.attr("x", - nodeRect.dx)
		.attr("y", - nodeRect.dy)
		.style("fill", d => d._children ? "lightsteelblue" : "lightgray")
		.attr("cursor", "pointer");

	const nodeExit = node.exit().transition()
		.duration(duration)
		.attr("transform", _ => `translate(${src.y}, ${src.x})`)
		.remove();
	// nodeExit.select("rect").attr("width", 1).attr("height", 1);
	nodeExit.select("text") .style("fill-opacity", 1e-6);

	/* LINKS */
	const link = svg.select("g").selectAll("path.link").data(links, d => d.id);
	const linkEnter = link.enter().insert("path", "g")
		.attr("class", "link")
		.attr("d", _ => {
			const o = { x: src.x0, y: src.y0 };
			return diagonal(o, o);
		});
	const linkUpdate = linkEnter.merge(link);
	linkUpdate.transition()
		.duration(duration)
		.attr("d", d => diagonal(d, d.parent));

	link.exit().transition()
		.duration(duration)
		.attr("d", _ => {
			const o = { x: src.x, y: src.y };
			return diagonal(o, o);
		}).remove();

	// store the old positions for transition
	nodes.forEach(d => {
		d.x0 = d.x;
		d.y0 = d.y;
	});

	// creates a curved (diagonal) path from parent to the child nodes
	function diagonal(s, d) {
		return `M ${s.y} ${s.x}
				C ${(s.y + d.y) / 2} ${s.x}, ${(s.y + d.y) / 2} ${d.x}, ${d.y} ${d.x}`;
	}

	// toggle children on click.
	function click(_, d) {
		if (d.children) {
			d._children = d.children;
			d.children = null;
		} else {
			d.children = d._children;
			d._children = null;
		}
		update(d);
	}
}
