<!-- I have no idea how to write PHP, exposing this to the Internet WILL get you hacked. -->
<!DOCTYPE html>
<html>
<head>
	<title>&#9856; &#9857; &#9858; &#9859; &#9860; &#9861;</title>
	<link rel="icon" type="image/x-icon" href="data:image/x-icon;base64,AAABAAEAEBAQAAEABAAoAQAAFgAAACgAAAAQAAAAIAAAAAEABAAAAAAAgAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAzMzMAAAA/wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgAgAgAgAgAiIiIiIiIiIAEBAQEBAQEAAAEBAQEBAAAAAQEBAQEAAAABAQEBAQAAAAABAQEAAAAAAAEBAQAAAAAAAQEBAAAAAAABAQEAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAD//wAA//8AAP//AAC22wAAAAEAAKqrAADqrwAA6q8AAOqvAAD6vwAA+r8AAPq/AAD6vwAA/v8AAP//AAD//wAA">
</head>
<body>
<input name="d" id="d" type="text" required style="width: 50%" value="<?php echo $_GET['d'] ?>" />
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
	if(! isset($_GET['d'])) {
		$command = "./roll -h";
	} else {
		$die = escapeshellarg($_GET['d']);
		if(isset($_GET['r'])) {
			$command = "./roll -v $die";
		} else {
			$command = "./roll -p $die";
		}
	}
	echo "$command";
	$output = shell_exec("$command 2>&1");
	echo "<pre>$output</pre>";
?>

</body>
</html>
