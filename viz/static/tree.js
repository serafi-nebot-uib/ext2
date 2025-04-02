d3.json("http://127.0.0.1:8000/data").then(data => {
	// Basic setup
	const scale = 75;
	const width = 16 * scale, height = 9 * scale;
	const margin = { top: 10, right: 120, bottom: 10, left: 120 };

	// Create a container with zoom capability
	const svg = d3.select(".container#inode-view > svg")
		.attr("viewBox", `0 0 ${width} ${height}`)
		.call(d3.zoom().on("zoom", (event) => {
			g.attr("transform", event.transform);
		}));

	// Main group that will be transformed on zoom/pan
	const g = svg.append("g")
		.attr("transform", `translate(${margin.left}, ${margin.top})`);

	const nodeSize = { width: 80, height: 40 };
	nodeSize.dx = nodeSize.width / 2;
	nodeSize.dy = nodeSize.height / 2;

	// Using nodeSize instead of size for better spacing
	const tree = d3.tree()
		.nodeSize([nodeSize.height * 1.5, nodeSize.width * 3])
		.separation((a, b) => {
			return (a.parent == b.parent ? 1.5 : 2) * 
				(a.parent ? Math.max(1, Math.log(a.parent.children.length) / Math.log(4)) : 1);
		});

	// Root setup
	const root = d3.hierarchy(data, d => d.children);
	root.x0 = 0;
	root.y0 = 0;

	// Collapse nodes initially
	root.children.forEach(collapse);

	// Function to collapse nodes
	function collapse(d) {
		if (!d.children) return;
		d._children = d.children;
		d._children.forEach(collapse);
		d.children = null;
	}

	// Initial pagination variables
	const childrenPerPage = 16;

	// Store pagination state for each node
	const nodePages = new Map();

	// Store the complete set of children for nodes with pagination
	const completeChildrenMap = new Map();

	// Function to handle pagination for a node
	function paginate(node, direction) {
		// If no page info exists for this node, initialize it
		if (!nodePages.has(node.id)) {
			nodePages.set(node.id, { currentPage: 0 });
		}

		// Make sure we have stored the complete children set
		if (!completeChildrenMap.has(node.id) && node._children) {
			completeChildrenMap.set(node.id, [...node._children]);
		}

		// Use the stored complete children set
		const allChildren = completeChildrenMap.get(node.id) || node._children;

		const pageInfo = nodePages.get(node.id);
		const totalPages = Math.ceil(allChildren.length / childrenPerPage);

		// Update current page based on direction
		if (direction === 'next') {
			pageInfo.currentPage = Math.min(totalPages - 1, pageInfo.currentPage + 1);
		} else if (direction === 'prev') {
			pageInfo.currentPage = Math.max(0, pageInfo.currentPage - 1);
		}

		// Calculate start and end indices for current page
		const start = pageInfo.currentPage * childrenPerPage;
		const end = Math.min(start + childrenPerPage, allChildren.length);

		// Update children array for display
		node.children = allChildren.slice(start, end);

		// Keep the complete set in _children
		node._children = allChildren;

		// Update the tree with new children
		update(node);
	}

	var i = 0, duration = 750;

	// Update tree visualization
	function update(src) {
		// Assign the x, y coords of each node
		const treeData = tree(root);

		// Calculate the new tree layout
		const nodes = treeData.descendants();
		const links = treeData.descendants().slice(1);

		// Normalize for fixed-depth
		nodes.forEach(d => { d.y = d.depth * 220 });

		/* NODES */
			const node = g.selectAll("g.node")
			.data(nodes, d => d.id || (d.id = ++i));

		// Node enter
		const nodeEnter = node.enter().append("g")
			.attr("class", "node")
			.attr("transform", _ => `translate(${src.y0}, ${src.x0})`)
			.on("click", (_, d) => {
				// Remove existing pagination controls when any node is clicked
				g.selectAll(".pagination-control").remove();

				if (d.children) {
					// FIXED: Store the complete set of children before collapsing
					if (d._children) {
						// If there's already a saved _children, use that complete set
						d._children = [...d._children];
					} else if (completeChildrenMap.has(d.id)) {
						// If we have a stored complete set, use that
						d._children = completeChildrenMap.get(d.id);
					} else {
						// Otherwise save the current children
						d._children = [...d.children];
					}
					d.children = null;
				} else if (d._children) {
					// Expand node
					// Store the complete set of children if not already stored
					if (!completeChildrenMap.has(d.id)) {
						completeChildrenMap.set(d.id, [...d._children]);
					}

					if (d._children.length > childrenPerPage) {
						// Initialize page info if not exists
						if (!nodePages.has(d.id)) {
							nodePages.set(d.id, { currentPage: 0 });
						}

						const pageInfo = nodePages.get(d.id);
						const allChildren = completeChildrenMap.get(d.id) || d._children;
						const start = pageInfo.currentPage * childrenPerPage;
						const end = Math.min(start + childrenPerPage, allChildren.length);

						// Show only the current page of children
						d.children = allChildren.slice(start, end);

						// Keep the complete set in _children
						d._children = allChildren;

						// Add pagination controls for this node after the tree update
						setTimeout(() => addPaginationControls(d), duration + 10);
					} else {
						// Show all children if less than the page size
						d.children = [...d._children];
					}
				}
				update(d);
			});

		// Add node rectangles
		nodeEnter.append("rect")
			.attr("class", "node")
			.attr("fill", "lightgray")
			.attr("stroke", "black");

		// Add count badge for collapsed nodes with many children
		nodeEnter.append("g")
			.attr("class", "count-badge")
			.attr("transform", _ => `translate(${nodeSize.dx}, ${-nodeSize.dy})`)
			.style("display", d => (d._children && d._children.length > 0) ? "block" : "none")
			.append("circle")
			.attr("r", 10)
			.attr("fill", "steelblue");

		nodeEnter.select(".count-badge")
			.append("text")
			.attr("text-anchor", "middle")
			.attr("dy", ".3em")
			.attr("fill", "white")
			.style("font-size", "8px")
			.text(d => {
				// Use the stored complete children set if available
				if (d._children && completeChildrenMap.has(d.id)) {
					return completeChildrenMap.get(d.id).length;
				}
				return d._children ? d._children.length : "";
			});

		// Add node text
		nodeEnter.append("text")
			.text(d => {
				return (d.data.name && d.data.name.length > 12) ? 
					d.data.name.substring(0, 10) + "..." : d.data.name;
			})
			.attr("class", "inode-title")
			.attr("text-anchor", "middle")
			.attr("dy", "0.3em");

		// Node update
		const nodeUpdate = nodeEnter.merge(node);

		nodeUpdate.transition()
			.duration(duration)
			.attr("transform", d => `translate(${d.y}, ${d.x})`);

		nodeUpdate.select("rect.node")
			.attr("width", nodeSize.width)
			.attr("height", nodeSize.height)
			.attr("x", -nodeSize.dx)
			.attr("y", -nodeSize.dy)
			.attr("rx", 5)
			.attr("ry", 5)
			.style("fill", d => d._children ? "lightsteelblue" : (d.children ? "#FFC107" : "lightgray"))
			.attr("cursor", "pointer");

		// Update count badges
		nodeUpdate.select(".count-badge")
			.style("display", d => (d._children && d._children.length > 0) ? "block" : "none")
			.select("text")
			.text(d => {
				// FIXED: Use the stored complete children set if available
				if (d._children && completeChildrenMap.has(d.id)) {
					return completeChildrenMap.get(d.id).length;
				}
				return d._children ? d._children.length : "";
			});

		// Node exit
		const nodeExit = node.exit().transition()
			.duration(duration)
			.attr("transform", _ => `translate(${src.y}, ${src.x})`)
			.style("opacity", 0)
			.remove();

		nodeExit.select("text").style("fill-opacity", 0);

		/* LINKS */
			const link = g.selectAll("path.link")
			.data(links, d => d.id);

		// Link enter
		const linkEnter = link.enter().insert("path", "g")
			.attr("class", "link")
			.attr("d", _ => {
				const o = { x: src.x0, y: src.y0 };
				return diagonal(o, o);
			})
			.attr("fill", "none")
			.attr("stroke", "#999")
			.attr("stroke-width", 1.5);

		// Link update
		const linkUpdate = linkEnter.merge(link);

		linkUpdate.transition()
			.duration(duration)
			.attr("d", d => diagonal(d, d.parent));

		// Link exit
		link.exit().transition()
			.duration(duration)
			.attr("d", _ => {
				const o = { x: src.x, y: src.y };
				return diagonal(o, o);
			})
			.remove();

		// Store the old positions for transition
		nodes.forEach(d => {
			d.x0 = d.x;
			d.y0 = d.y;
		});

		// Creates a curved (diagonal) path from parent to the child nodes
		function diagonal(s, d) {
			return `M ${s.y} ${s.x}
			C ${(s.y + d.y) / 2} ${s.x}, ${(s.y + d.y) / 2} ${d.x}, ${d.y} ${d.x}`;
		}

		// Update pagination controls position if they exist
		updatePaginationControlPositions(nodes);
	}

	// Function to update pagination control positions
	// FIXED: Added nodes parameter
	function updatePaginationControlPositions(nodes) {
		// Find all nodes with pagination
		nodes.forEach(d => {
			if (d.children && d._children && d._children.length > childrenPerPage) {
				// Update position of existing pagination controls
				const controls = g.select(`.pagination-control[data-parent="${d.id}"]`);
				if (!controls.empty()) {
					controls.attr("transform", `translate(${d.y}, ${d.x + nodeSize.height})`);

					// Update page indicator
					if (nodePages.has(d.id)) {
						const pageInfo = nodePages.get(d.id);
						const totalChildren = completeChildrenMap.has(d.id) ? 
							completeChildrenMap.get(d.id).length : d._children.length;
						const totalPages = Math.ceil(totalChildren / childrenPerPage);
						controls.select(`#page-indicator-${d.id}`)
							.text(`${pageInfo.currentPage + 1}/${totalPages}`);
					}
				}
			}
		});
	}

	// Function to add pagination controls
	function addPaginationControls(d) {
		const totalChildren = completeChildrenMap.has(d.id) ? 
			completeChildrenMap.get(d.id).length : (d._children ? d._children.length : 0);

		if (totalChildren <= childrenPerPage) return;

		// Remove any existing pagination controls for this node
		g.selectAll(`.pagination-control[data-parent="${d.id}"]`).remove();

		// Initialize page info if doesn't exist
		if (!nodePages.has(d.id)) {
			nodePages.set(d.id, { currentPage: 0 });
		}

		const pageInfo = nodePages.get(d.id);
		const totalPages = Math.ceil(totalChildren / childrenPerPage);

		// Create pagination container
		const paginationG = g.append("g")
			.attr("class", "pagination-control")
			.attr("data-parent", d.id)
			.attr("transform", `translate(${d.y}, ${d.x + nodeSize.height})`);

		// Previous button
		paginationG.append("rect")
			.attr("width", 20)
			.attr("height", 20)
			.attr("x", -40)
			.attr("y", 10)
			.attr("fill", "white")
			.attr("stroke", "black")
			.attr("cursor", "pointer")
			.on("click", (event) => {
				event.stopPropagation(); // Prevent node click from triggering
				paginate(d, 'prev');
			});

		paginationG.append("text")
			.attr("x", -30)
			.attr("y", 25)
			.text("<")
			.attr("pointer-events", "none");

		// Next button
		paginationG.append("rect")
			.attr("width", 20)
			.attr("height", 20)
			.attr("x", 20)
			.attr("y", 10)
			.attr("fill", "white")
			.attr("stroke", "black")
			.attr("cursor", "pointer")
			.on("click", (event) => {
				event.stopPropagation(); // Prevent node click from triggering
				paginate(d, 'next');
			});

		paginationG.append("text")
			.attr("x", 30)
			.attr("y", 25)
			.text(">")
			.attr("pointer-events", "none");

		// Page indicator
		paginationG.append("text")
			.attr("id", `page-indicator-${d.id}`)
			.attr("x", 0)
			.attr("y", 25)
			.attr("text-anchor", "middle")
			.text(`${pageInfo.currentPage + 1}/${totalPages}`);

		// Background to prevent click-through
		paginationG.insert("rect", ":first-child")
			.attr("width", 80)
			.attr("height", 40)
			.attr("x", -40)
			.attr("y", 0)
			.attr("fill", "white")
			.attr("opacity", 0.5)
			.attr("pointer-events", "none");
	}

	// Initial update
	update(root);
});
