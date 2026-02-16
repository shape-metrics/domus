const ERR_TOO_LARGE = -1;
const ERR_DISCONNECTED = -2;
const ERR_UNKNOWN = -3;

const INPUT_FILE = 'input.txt';
const OUTPUT_SVG_FILE = 'output.svg';
const OUTPUT_GRAPHML_FILE = 'output.graphml';

var Module = {
    onRuntimeInitialized: function() {
        // Populate example graphs dropdown
        const select = document.getElementById('example-graphs');
        FS.readdir('/example-graphs').filter(
            f => f.endsWith('.txt')).forEach(
            (graphFile) => {
                const option = document.createElement('option');
                option.value = graphFile;
                option.textContent = graphFile;
                select.appendChild(option);
        });
        document.querySelector('form').addEventListener('submit', (event) => {
            event.preventDefault();
            const selectedGraph = select.value;
            const inputLabel = document.getElementById('graph-upload');
            inputLabel.textContent = selectedGraph;
            const graphPath = `/example-graphs/${selectedGraph}`;
            const graphContent = FS.readFile(graphPath, { encoding: 'utf8' });
            FS.writeFile(INPUT_FILE, graphContent);
        });
    }
};

function download(path, filename) {
    const content = FS.readFile(path, { encoding: 'utf8' });
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

function downloadExample() {
    const graphName = document.getElementById('example-graphs').value;
    download(`/example-graphs/${graphName}`, graphName);
}

function fitSvg(svg, padding = 8) {
    if (!svg) return;
    try {
        let group = svg.querySelector('g');
        if (group) {
            const b = group.getBBox();
            const x = b.x - padding, y = b.y - padding;
            const w = Math.max(1, b.width + padding * 2);
            const h = Math.max(1, b.height + padding * 2);
            svg.setAttribute('viewBox', `${x} ${y} ${w} ${h}`);
        } else {
            const children = Array.from(svg.children).filter(el => el.tagName.toLowerCase() !== 'defs');
            let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
            children.forEach(el => {
            if (typeof el.getBBox === 'function') {
                try {
                const bb = el.getBBox();
                minX = Math.min(minX, bb.x);
                minY = Math.min(minY, bb.y);
                maxX = Math.max(maxX, bb.x + bb.width);
                maxY = Math.max(maxY, bb.y + bb.height);
                } catch (e) {}
            }
            });
            if (minX === Infinity) return;
            const x = minX - padding, y = minY - padding;
            const w = Math.max(1, (maxX - minX) + padding * 2);
            const h = Math.max(1, (maxY - minY) + padding * 2);
            svg.setAttribute('viewBox', `${x} ${y} ${w} ${h}`);
        }
    } catch (err) {
        const rect = svg.getBoundingClientRect();
        svg.setAttribute('viewBox', `0 0 ${rect.width || 800} ${rect.height || 600}`);
    }
    svg.setAttribute('preserveAspectRatio', 'xMidYMid meet');
    svg.style.width = '90%';
    svg.style.height = '90%';
}

function visualizeSvg(fileName, svgElement) {
    const svgContent = FS.readFile(fileName, { encoding: 'utf8' });
    svgElement.innerHTML = svgContent;
    // wait a frame so the inserted SVG is parsed, then compute the viewBox
    requestAnimationFrame(() => {
        const svg = svgElement.querySelector('svg');
        if (svg) fitSvg(svg, 8);
    });
}
    
document.getElementById('fileInput')
.addEventListener('change', (event) => {
    const file = event.target.files[0];
    const reader = new FileReader();
    reader.onload = function (e) {
        FS.writeFile(INPUT_FILE, e.target.result);
    };
    reader.readAsText(file);
    document.getElementById('graph-upload').textContent = file.name;
});

function computeDrawing() {
    const value = Module.ccall('compute_orthogonal_drawing', null, [], []);
    if (value === ERR_TOO_LARGE) {
        alert('Error: The graph is too large (more than 30 vertices).');
        return;
    } else if (value === ERR_DISCONNECTED) {
        alert('Error: The graph is disconnected.');
        return;
    } else if (value === ERR_UNKNOWN) {
        alert('Error: An unexpected error occurred during computation.');
        return;
    }
    visualizeSvg(OUTPUT_SVG_FILE, document.getElementById('drawing-svg'));
    let buttons = [];
    buttons.push(document.getElementById("download-graphml-id"));
    buttons.push(document.getElementById("download-svg-id"));
    buttons.forEach((button) => {
        button.disabled = false;
        button.classList.add('enabled');
    });
}

document.getElementById('download-svg-id')
.addEventListener('click', (_) => {
    download(OUTPUT_SVG_FILE, "drawing.svg");
});

document.getElementById('download-graphml-id')
.addEventListener('click', (_) => {
    download(OUTPUT_GRAPHML_FILE, "graph.graphml");
});