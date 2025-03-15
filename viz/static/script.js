const width = 400, height = 50;
const padding = 5;
const headerWidth = width / 10;
const headerHeight = height - padding*2;

const svg = d3.select("svg").attr("viewBox", `0 0 ${width} ${height}`);
const g = svg.append("g");

const sbRect = g.append("rect")
	.attr("class", "fs-section-box")
	.attr("id", "superblock")
	.attr("x", padding)
	.attr("y", padding)
	.attr("width", headerWidth)
	.attr("height", headerHeight)
	.attr("fill", "#4287f5")
	.attr("stroke", "black")
	.attr("stroke-width", "1")

const sbTitle = g.append("text")
	.text("Superblock")
	.attr("class", "fs-section-title")
	.attr("x", padding + headerHeight / 2)
	.attr("y", padding + headerWidth / 2)
	.attr("transform", `rotate(-90, ${padding + headerHeight / 2}, ${padding + headerWidth / 2})`);

const bmRect = g.append("rect")
	.attr("class", "fs-section-box")
	.attr("id", "blockmap")
	.attr("x", headerWidth + padding)
	.attr("y", padding)
	.attr("width", headerWidth)
	.attr("height", headerHeight)
	.attr("fill", "#8f00ff");

const bmTitle = g.append("text")
	.text("Block Map")
	.attr("class", "fs-section-title")
	.attr("x", padding + headerHeight / 2)
	.attr("y", padding + headerWidth * (1/2 + 1))
	.attr("transform", `rotate(-90, ${padding + headerHeight / 2}, ${padding + headerWidth / 2})`);

const inodeRect = g.append("rect")
	.attr("class", "fs-section-box")
	.attr("id", "inode")
	.attr("x", headerWidth*2 + padding)
	.attr("y", padding)
	.attr("width", headerWidth)
	.attr("height", headerHeight)
	.attr("fill", "#3b7a57");

const inodeTitle = g.append("text")
	.text("Inodes")
	.attr("class", "fs-section-title")
	.attr("x", padding + headerHeight / 2)
	.attr("y", padding + headerWidth * (1/2 + 2))
	.attr("transform", `rotate(-90, ${padding + headerHeight / 2}, ${padding + headerWidth / 2})`);

const dataRect = g.append("rect")
	.attr("class", "fs-section-box")
	.attr("id", "data")
	.attr("x", headerWidth*3 + padding)
	.attr("y", padding)
	.attr("width", headerWidth*7-padding*2)
	.attr("height", headerHeight)
	.attr("fill", "#FF8C00");

const dataTitle = g.append("text")
	.text("Data")
	.attr("class", "fs-section-title")
	.attr("x", padding + headerWidth * (1/2 + 6))
	.attr("y", padding + headerHeight / 2);

g.selectAll("rect.fs-section-box")
	.on("mouseover", (e, d) => {
		d3.select(e.currentTarget).attr("fill-opacity", "0.5");
	})
	.on("mouseout", (e, d) => {
		d3.select(e.currentTarget).attr("fill-opacity", "1");
	});

function resize_svg() {
	const svg = d3.select("svg");
	const container = svg.node().parentNode;
	const w = container.clientWidth;
	const aspectRatio = width / height;
	const h = w / aspectRatio;
	svg.attr("width", w).attr("height", h);
}

window.addEventListener("resize", resize_svg);

resize_svg();
