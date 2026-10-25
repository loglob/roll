<!-- I have no idea how to write PHP, exposing this to the Internet WILL get you hacked. -->
<!DOCTYPE html>
<html>
<head>
	<title>&#9856; &#9857; &#9858; &#9859; &#9860; &#9861;</title>
	<link rel="icon" type="image/x-icon" href="data:image/x-icon;base64,AAABAAEAEBAQAAEABAAoAQAAFgAAACgAAAAQAAAAIAAAAAEABAAAAAAAgAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAzMzMAAAA/wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgAgAgAgAgAiIiIiIiIiIAEBAQEBAQEAAAEBAQEBAAAAAQEBAQEAAAABAQEBAQAAAAABAQEAAAAAAAEBAQAAAAAAAQEBAAAAAAABAQEAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAD//wAA//8AAP//AAC22wAAAAEAAKqrAADqrwAA6q8AAOqvAAD6vwAA+r8AAPq/AAD6vwAA/v8AAP//AAD//wAA">
</head>
<body>
<input name="d" id="d" type="text" required style="width: 50%" value="<?php echo htmlspecialchars($_GET['d'] ?? '') ?>" />
<script>
	const d = document.getElementById('d')
	function submit(r) {
		location.replace(`roll.php?${(r ? 'r&' : '')}d=${encodeURIComponent(d.value)}`)
	}
	d.onkeydown = function(ev) {
		// 13 is enter
		if(ev.keyCode == 13) {
			submit(false)
			return false
		}
	}
</script>
<br/>
<button onclick="submit(true)"> Roll </button>
<button onclick="submit(false)"> Histogram </button>
<br/>
<?php
	if(!isset($_GET['d']) || $_GET['d'] === '') {
		$command = ['./roll', '-h'];
	} else if(isset($_GET['r'])) {
		$command = ['./roll', '-v', $_GET['d']];
	} else {
		$command = ['./roll', '-p', $_GET['d']];
	}
	echo htmlspecialchars(implode(' ', $command));
	// passing an array bypasses the shell; stderr is merged into stdout like 2>&1
	$proc = proc_open($command, [1 => ['pipe', 'w'], 2 => ['redirect', 1]], $pipes);
	$output = stream_get_contents($pipes[1]);
	fclose($pipes[1]);
	proc_close($proc);
	echo '<pre style="font-size: 120%;">' . htmlspecialchars($output) . '</pre>';
?>
<footer style="color: rgb(178, 172, 162); margin-top: 3em; font-size: 75%;">
	roll is provided under the GPL-3.0 license without warranty. Source code is available <a href="https://github.com/loglob/roll">here</a>.
</footer>
</body>
</html>
