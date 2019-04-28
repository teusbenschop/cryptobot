<?php

  $url = "https://api.bl3p.eu/1/";
  $pubkey = "--enter--your--value--here--";
  $privkey = "--enter--your--value--here--";

  $path = "BTCEUR/money/deposit_address";

  $params = array();

  $mt = explode(' ', microtime());
  $params['nonce'] = $mt[1].substr($mt[0], 2, 6);
  
  $post_data = http_build_query($params, '', '&');
  $body = $path . chr(0). $post_data;
  
  $sign = base64_encode(hash_hmac('sha512', $body, base64_decode($privkey), true));
  
  $fullpath = $url . $path;
  
  $headers = array(
                   'Rest-Key: '.$pubkey,
                   'Rest-Sign: '. $sign,
                   );
  
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
  curl_setopt($ch, CURLOPT_URL, $fullpath);
  curl_setopt($ch, CURLOPT_POSTFIELDS, $post_data);
  curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
  curl_setopt($ch, CURLOPT_VERBOSE, true);

  $res = curl_exec($ch);
  
  curl_close ($ch);

  var_dump ($res);
  
?>
